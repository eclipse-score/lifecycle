/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/launch_manager/src/execution_error.h"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/log.hpp"

namespace score::mw::lifecycle::internal
{

namespace
{

using score::mw::lifecycle::internal::configuration::ApplicationType;

bool isSupervised(const ApplicationType& application_type) noexcept
{
    switch (application_type)
    {
        case ApplicationType::Native:
        case ApplicationType::Reporting:
            return false;
        case ApplicationType::ReportingAndSupervised:
        case ApplicationType::StateManager:
            return true;
    }

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("All possible cases are covered");
}

std::unique_ptr<IAliveSupervisionHandle> constructSupervision(
    configuration::ComponentConfig& config,
    ISupervisionFactory& supervision_factory)
{
    if (!isSupervised(config.component_properties.application_profile.application_type))
    {
        return nullptr;
    }

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        app_profile.alive_supervision.has_value(), "Supervised component did not have alive supervision config");

    LM_LOG_DEBUG() << "Setting up alive supervision for" << config.name;
    std::unique_ptr<IAliveSupervisionHandle> handle = supervision_factory.constructSupervision(
        IdentifierHash{config.name},
        config.deployment_config.sandbox.uid,
        config.component_properties.application_profile.alive_supervision.value());
    if (!handle)
    {
        LM_LOG_ERROR() << "Failed to set up alive supervision for" << config.name;
        return nullptr;
    }

    config.deployment_config.environmental_variables.add("LCM_ALIVE_INTERFACE_PATH", handle->getConnectionId());
    return handle;
}

}  // namespace

// codeql[cpp/number-of-parameters-per-function] Dismiss: Using a struct for the parameters wouldn't make this easier to read.
Component::Component(
    std::unique_ptr<const IStartAction>&& start_action,
    std::unique_ptr<const IStopAction>&& stop_action,
    std::unique_ptr<const IForceStopAction>&& force_stop_action,
    std::unique_ptr<const IReadyCondition>&& ready_condition,
    ISupervisionFactory& supervision_factory,
    std::shared_ptr<configuration::ComponentConfig>&& config)
    : start_action_(std::move(start_action)),
      stop_action_(std::move(stop_action)),
      force_stop_action_(std::move(force_stop_action)),
      ready_condition_(std::move(ready_condition)),
      config_(std::move(config)),
      supervision_handle_(constructSupervision(*config_, supervision_factory)),
      state_(TerminatedState{})
{
}

IComponent::RequestResult Component::activate(cpp::stop_token stop_token)
{
    const RequestResult result = tryActivate(stop_token);
    if (result != RequestResult::kSuccess)
    {
        // A failed activation must not leave anything running.
        stopAfterFailedActivation();
    }
    return result;
}

IComponent::RequestResult Component::tryActivate(cpp::stop_token stop_token)
{
    const std::optional<Handle> handle = beginActivation(stop_token);
    if (!handle.has_value())
    {
        return RequestResult::kFailure;
    }

    // The state is deliberately not locked while waiting for the ready condition: the wait may take
    // up to the ready timeout, and terminations must be reported to the ready condition meanwhile.
    // Some ready conditions can only be woken up by a termination.
    const Result<void> ready = ready_condition_->wait(stop_token, handle.value());

    return completeActivation(ready);
}

std::optional<Handle> Component::beginActivation(cpp::stop_token stop_token)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        std::holds_alternative<TerminatedState>(state_), "Cannot activate component in a running state");

    if (isSupervised(config_->component_properties.application_profile.application_type) && !supervision_handle_)
    {
        LM_LOG_ERROR() << config_->name << ": alive supervision is not set up";
        return std::nullopt;
    }

    // Anything left over from a previous activation is dropped.
    ready_condition_->prepare();

    // The resource is started while the state is locked, so that a termination reported right after
    // it is tracked finds StartingState.
    const Result<StartedResource> start_result = start_action_->start(stop_token, *this);
    if (!start_result.has_value())
    {
        return std::nullopt;
    }
    const StartedResource& started = start_result.value();

    state_ = StartingState{started.handle, false};

    // If the resource terminated before it could be tracked, its termination is not reported
    // later, so it is handled here, while the state is still locked. During startup it is always
    // ignored: the ready condition and completeActivation() decide about the activation.
    if (started.exit_status.has_value())
    {
        static_cast<void>(tryHandleTerminationInternally(started.exit_status.value()));
    }

    return started.handle;
}

IComponent::RequestResult Component::completeActivation(const Result<void>& ready)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    // Only a termination can run concurrently to the activation, and it keeps StartingState.
    const StartingState* const starting = std::get_if<StartingState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(starting != nullptr, "Component must still be starting");

    if (starting->terminated_unexpectedly_)
    {
        LM_LOG_WARN() << config_->name << ": terminated unexpectedly during startup";
        return RequestResult::kFailure;
    }

    if (!ready.has_value())
    {
        if (ready.error() == ExecErrc::kTimeout)
        {
            LM_LOG_WARN() << config_->name << ": ready condition was not met within the ready timeout";
            return RequestResult::kTimeout;
        }
        return RequestResult::kFailure;
    }

    const std::optional<std::chrono::nanoseconds> supervision_start = activateSupervision();
    if (!supervision_start.has_value())
    {
        return RequestResult::kFailure;
    }

    const Handle handle = starting->handle_;
    state_ = ReadyState{handle, supervision_start.value()};
    return RequestResult::kSuccess;
}

void Component::stopAfterFailedActivation()
{
    std::unique_lock<std::mutex> lock{state_mutex_};

    // Nothing was started.
    if (std::holds_alternative<TerminatedState>(state_))
    {
        return;
    }

    const StartingState* const starting = std::get_if<StartingState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        starting != nullptr, "Component must still be starting after a failed activation");

    const Handle handle = starting->handle_;
    static_cast<void>(stopAndWait(lock, handle));
}

bool Component::stopAndWait(std::unique_lock<std::mutex>& lock, const Handle& handle)
{
    // Stopping cannot be interrupted, so no stop token is passed on.
    const Result<RequestResult> stopped = stop_action_->stop(cpp::stop_token{}, handle);

    // Without a resource, or if its termination was already handled, no termination will be reported
    // anymore, so there is nothing to wait for.
    if (!isAlive(handle))
    {
        state_ = TerminatedState{};
        return true;
    }

    // If no stop request reached the resource, no termination can be waited for, and a forced stop is
    // not tried either.
    if (!stopped.has_value() || (stopped.value() != RequestResult::kSuccess))
    {
        LM_LOG_ERROR() << config_->name << ": failed to request the resource to stop";
        state_ = TerminatedState{};
        return false;
    }

    state_ = TerminatingState{handle};
    if (waitUntilTerminated(lock, std::chrono::milliseconds{config_->deployment_config.shutdown_timeout_ms}))
    {
        return true;
    }

    LM_LOG_WARN() << config_->name << ": did not terminate within the shutdown timeout, stopping it forcefully";
    const Result<RequestResult> forced = force_stop_action_->forceStop(cpp::stop_token{}, handle);
    if (forced.has_value() && (forced.value() == RequestResult::kSuccess) &&
        waitUntilTerminated(lock, kMaxSigKillDelay))
    {
        return true;
    }

    LM_LOG_ERROR() << config_->name << ": failed to stop the resource forcefully";
    state_ = TerminatedState{};
    return false;
}

bool Component::waitUntilTerminated(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds timeout)
{
    return termination_handled_.wait_for(lock, timeout, [this] {
        return !std::holds_alternative<TerminatingState>(state_);
    });
}

IComponent::RequestResult Component::deactivate(cpp::stop_token /*stop_token*/)
{
    std::unique_lock<std::mutex> lock{state_mutex_};

    if (std::holds_alternative<TerminatedState>(state_))
    {
        return RequestResult::kSuccess;
    }

    const ReadyState* const ready = std::get_if<ReadyState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        ready != nullptr, "Cannot deactivate component while it is activating or deactivating");
    const Handle handle = ready->handle_;

    // A failure to deactivate supervision must not prevent stopping the resource.
    static_cast<void>(deactivateSupervision());

    return stopAndWait(lock, handle) ? RequestResult::kSuccess : RequestResult::kFailure;
}

IComponent::TerminationResult Component::tryHandleTermination(int32_t status)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};
    const TerminationResult result = tryHandleTerminationInternally(status);
    // Notified while still locked: once stopAndWait() sees the termination, the component may be
    // destroyed, so this thread must not touch it after releasing the lock.
    termination_handled_.notify_all();
    return result;
}

IComponent::TerminationResult Component::tryHandleTerminationInternally(int32_t status)
{
    // If we are waiting for termination, mark it as done. We do not care
    // about the exit code.
    if (std::holds_alternative<TerminatingState>(state_))
    {
        state_ = TerminatedState{};
        return TerminationResult::kIgnore;
    }

    // The component already gave up on the resource, e.g. after a failed cleanup.
    if (std::holds_alternative<TerminatedState>(state_))
    {
        LM_LOG_DEBUG() << config_->name << ": ignoring termination of a resource which is no longer tracked";
        return TerminationResult::kIgnore;
    }

    StartingState* const starting = std::get_if<StartingState>(&state_);
    ReadyState* const ready = std::get_if<ReadyState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        (starting != nullptr) || (ready != nullptr), "Cannot handle termination in this state");
    Handle& handle = (starting != nullptr) ? starting->handle_ : ready->handle_;

    // Invalidate the resource to prevent acting on something which is already gone.
    invalidate(handle);

    if (starting != nullptr)
    {
        // The ready condition decides whether its wait succeeds, the component decides whether the
        // termination is expected. activate() reports the outcome, so there is only one event.
        ready_condition_->handleTermination(handle, status);
        starting->terminated_unexpectedly_ = !isExpectedTermination(status);
        return TerminationResult::kIgnore;
    }

    // A failure to deactivate supervision does not change the outcome: the resource is gone anyway.
    static_cast<void>(deactivateSupervision());

    if (isExpectedTermination(status))
    {
        return TerminationResult::kIgnore;
    }

    state_ = TerminatedState{};
    return TerminationResult::kFailure;
}

bool Component::isExpectedTermination(int32_t status) const
{
    return config_->component_properties.application_profile.is_self_terminating && (status == 0);
}

void Component::handleSupervisionFailure(std::chrono::nanoseconds failure_timestamp)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        supervision_handle_ != nullptr, "Non supervised component received a supervision failure event");

    const ReadyState* const ready = std::get_if<ReadyState>(&state_);
    if (ready == nullptr)
    {
        // The component is not ready, it may be deactivated or currently activating or deactivating.
        return;
    }

    if (ready->supervision_start_ >= failure_timestamp)
    {
        // The supervision failure is from a previous activation.
        return;
    }

    if (!isAlive(ready->handle_))
    {
        return;
    }

    LM_LOG_WARN() << "Alive supervision failed for component" << config_->name << ", stopping it forcefully";
    static_cast<void>(force_stop_action_->forceStop(cpp::stop_token{}, ready->handle_));
}

void Component::forceStop()
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    const Handle* handle = nullptr;
    if (const StartingState* const starting = std::get_if<StartingState>(&state_))
    {
        handle = &starting->handle_;
    }
    else if (const ReadyState* const ready = std::get_if<ReadyState>(&state_))
    {
        handle = &ready->handle_;
    }
    else if (const TerminatingState* const terminating = std::get_if<TerminatingState>(&state_))
    {
        handle = &terminating->handle_;
    }

    if ((handle != nullptr) && isAlive(*handle))
    {
        static_cast<void>(force_stop_action_->forceStop(cpp::stop_token{}, *handle));
    }
}

IdentifierHash Component::getIdentifier() const
{
    return IdentifierHash{config_->name};
}

bool Component::active() const
{
    const std::lock_guard<std::mutex> lock{state_mutex_};
    return std::holds_alternative<ReadyState>(state_);
}

std::optional<std::chrono::nanoseconds> Component::activateSupervision() const
{
    if (!supervision_handle_)
    {
        return std::chrono::nanoseconds{0};
    }

    timespec timestamp{};

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
        LM_LOG_ERROR() << config_->name << ": failed to get timestamp to activate supervision:" << errno_message(errno);
        return std::nullopt;
    }

    if (!supervision_handle_->activateSupervision(timestamp))
    {
        LM_LOG_ERROR() << config_->name << ": failed to activate supervision";
        return std::nullopt;
    }

    return std::chrono::seconds{timestamp.tv_sec} + std::chrono::nanoseconds{timestamp.tv_nsec};
}

bool Component::deactivateSupervision() const
{
    if (!supervision_handle_)
    {
        return true;
    }

    timespec timestamp{};

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
        LM_LOG_ERROR() << config_->name
                       << ": failed to get timestamp to deactivate supervision: " << errno_message(errno);
        return false;
    }

    if (!supervision_handle_->deactivateSupervision(timestamp))
    {
        LM_LOG_ERROR() << config_->name << ": failed to deactivate supervision";
        return false;
    }

    return true;
}

}  // namespace score::mw::lifecycle::internal

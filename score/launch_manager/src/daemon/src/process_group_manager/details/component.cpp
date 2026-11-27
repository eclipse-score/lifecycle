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
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/configuration/environment_config.hpp"
#include "score/result/error.h"
#include "score/result/result.h"
#include <score/assert.hpp>
#include <errno.h>
#include <time.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#ifndef LC_LOG_SCORE_MW_LOG
#include <ostream>  // Only needed by the std::cout logger variant of operator<<(ComponentId).
#endif              // LC_LOG_SCORE_MW_LOG

namespace score::mw::lifecycle::internal
{

namespace
{

using score::mw::lifecycle::internal::configuration::ApplicationType;

/// @brief Identifies a component in logs as "Component: <name>", followed by its handle if it has one.
struct ComponentId
{
    std::string_view name;
    const Handle* handle{nullptr};
};

template <class Stream>
Stream& operator<<(Stream& stream, const ComponentId& component_id)
{
    stream << "Component:" << component_id.name;
    if (component_id.handle != nullptr)
    {
        stream << *component_id.handle;
    }
    return stream;
}

#ifndef LC_LOG_SCORE_MW_LOG
/// @brief Variant for the std::cout logger, which writes the ComponentId as a single argument: unlike the log
///        stream, std::ostream does not separate what is written to it, so the spaces are added here.
std::ostream& operator<<(std::ostream& stream, const ComponentId& component_id)
{
    stream << "Component: " << component_id.name;
    if ((component_id.handle != nullptr) && isAlive(*component_id.handle))
    {
        stream << ' ' << *component_id.handle;
    }
    return stream;
}
#endif  // LC_LOG_SCORE_MW_LOG

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
        config.component_properties.application_profile.alive_supervision.has_value(),
        "Supervised component did not have alive supervision config");

    LM_LOG_DEBUG() << ComponentId{config.name} << "setting up alive supervision";
    std::unique_ptr<IAliveSupervisionHandle> handle = supervision_factory.constructSupervision(
        IdentifierHash{config.name},
        config.deployment_config.sandbox.uid,
        config.component_properties.application_profile.alive_supervision.value());
    if (!handle)
    {
        LM_LOG_ERROR() << ComponentId{config.name} << "failed to set up alive supervision";
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
    const std::uint32_t attempts = 1U + config_->deployment_config.activation_retry_attempts;
    const std::chrono::steady_clock::time_point activation_start = std::chrono::steady_clock::now();

    RequestResult result{RequestResult::kFailure};
    for (std::uint32_t attempt = 1U; attempt <= attempts; ++attempt)
    {
        LM_LOG_DEBUG() << ComponentId{config_->name} << "starting activation attempt" << attempt << "of" << attempts;
        result = tryActivate(stop_token, activation_start);
        if (result == RequestResult::kSuccess)
        {
            return result;
        }

        // A failed attempt must not leave anything running. If the resource may still be running, another
        // attempt could start a second instance of it.
        if (!stopAfterFailedActivation())
        {
            LM_LOG_ERROR() << ComponentId{config_->name} << "activation attempt" << attempt << "of" << attempts
                           << "failed and the resource could not be stopped, not retrying";
            return result;
        }

        if (stop_token.stop_requested())
        {
            LM_LOG_DEBUG() << ComponentId{config_->name} << "activation was cancelled, not retrying";
            return result;
        }

        if (attempt < attempts)
        {
            LM_LOG_WARN() << ComponentId{config_->name} << "activation attempt" << attempt << "of" << attempts
                          << "failed, retrying";
        }
    }

    LM_LOG_ERROR() << ComponentId{config_->name} << "activation failed after" << attempts << "attempt(s)";
    return result;
}

IComponent::RequestResult Component::tryActivate(
    cpp::stop_token stop_token,
    std::chrono::steady_clock::time_point activation_start)
{
    const std::optional<Handle> handle = beginActivation(stop_token);
    if (!handle.has_value())
    {
        return RequestResult::kFailure;
    }

    // The state is deliberately not locked while waiting for the ready condition: the wait may take
    // up to the ready timeout, and terminations must be reported to the ready condition meanwhile.
    // Some ready conditions can only be woken up by a termination.
    const Result<void> ready_condition_result = ready_condition_->wait(
        stop_token, handle.value(), std::chrono::milliseconds{config_->deployment_config.ready_timeout_ms});

    const RequestResult result = completeActivation(ready_condition_result, stop_token.stop_requested());
    if (result == RequestResult::kSuccess)
    {
        const std::chrono::milliseconds duration =
            std::chrono::round<std::chrono::milliseconds>(std::chrono::steady_clock::now() - activation_start);

        // req-id: comp_req__launch_man__process_log
        LM_LOG_INFO() << ComponentId{config_->name, &handle.value()} << "is ready, activation took" << duration.count()
                       << "ms";
    }
    return result;
}

std::optional<Handle> Component::beginActivation(cpp::stop_token stop_token)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        std::holds_alternative<TerminatedState>(state_), "Cannot activate component in a running state");

    if (isSupervised(config_->component_properties.application_profile.application_type) && !supervision_handle_)
    {
        LM_LOG_ERROR() << ComponentId{config_->name} << "alive supervision could not be set up, aborting activation";
        return std::nullopt;
    }

    // Anything left over from a previous activation is dropped.
    ready_condition_->prepare();

    // The component is starting from here on. The handle is only known once the resource is started.
    state_ = StartingState{Handle{}, false};

    // The resource is started while the state is locked, so that a termination reported right after
    // it is tracked finds StartingState.
    const Result<StartedResource> start_result = start_action_->start(stop_token, *this);
    if (!start_result.has_value())
    {
        LM_LOG_WARN() << ComponentId{config_->name} << "failed to start:" << start_result.error().Message();
        // Nothing is left running, see IStartAction::start().
        state_ = TerminatedState{};
        return std::nullopt;
    }
    const StartedResource& started = start_result.value();
    state_ = StartingState{started.handle, false};

    // The resource may have terminated directly after startup, in which case its termination is handled immediately.
    if (started.exit_status.has_value())
    {
        static_cast<void>(tryHandleTerminationInternally(started.handle, started.exit_status.value()));
    }

    return started.handle;
}

IComponent::RequestResult Component::completeActivation(const Result<void>& ready_condition_result, bool cancelled)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    // Only a termination can run concurrently to the activation, and it keeps StartingState.
    const StartingState* const starting = std::get_if<StartingState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(starting != nullptr, "Component must still be starting");

    if (starting->terminated_unexpectedly_)
    {
        return RequestResult::kFailure;
    }

    if (!ready_condition_result.has_value())
    {
        if (ready_condition_result.error() == ExecErrc::kTimeout)
        {
            LM_LOG_WARN() << ComponentId{config_->name, &starting->handle_}
                          << "did not meet its ready condition within the ready timeout";
            return RequestResult::kTimeout;
        }
        if (cancelled)
        {
            LM_LOG_DEBUG() << ComponentId{config_->name, &starting->handle_}
                           << "activation was cancelled while waiting for the ready condition";
        }
        else
        {
            LM_LOG_WARN() << ComponentId{config_->name, &starting->handle_}
                          << "failed its ready condition:" << ready_condition_result.error().Message();
        }
        return RequestResult::kFailure;
    }

    std::chrono::nanoseconds supervision_start{0};
    if (isAlive(starting->handle_))
    {
        const std::optional<std::chrono::nanoseconds> started = activateSupervision();
        if (!started.has_value())
        {
            return RequestResult::kFailure;
        }
        supervision_start = started.value();
    }

    const Handle handle = starting->handle_;
    state_ = ReadyState{handle, supervision_start};
    return RequestResult::kSuccess;
}

bool Component::stopAfterFailedActivation()
{
    std::unique_lock<std::mutex> lock{state_mutex_};

    // Nothing was started.
    if (std::holds_alternative<TerminatedState>(state_))
    {
        return true;
    }

    const StartingState* const starting = std::get_if<StartingState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        starting != nullptr, "Component must still be starting after a failed activation");

    const Handle handle = starting->handle_;
    // Stopping cannot be interrupted, so no stop token is passed on.
    return stopAndWait(lock, handle);
}

bool Component::stopAndWait(std::unique_lock<std::mutex>& lock, const Handle& handle)
{
    // This code has to run while holding state_mutex_ so that `waitUntilTerminated` is invoked
    // before a termination can be reported.

    state_ = TerminatingState{handle};
    const Result<RequestResult> stopped = stop_action_->stop(cpp::stop_token{}, handle);

    // Without a resource, or if its termination was already handled, no termination will be reported
    // anymore, so there is nothing to wait for.
    if (!isAlive(handle))
    {
        state_ = TerminatedState{};
        return true;
    }

    // A stop request also fails if the resource has already terminated, but its termination is not
    // handled yet, e.g. because it waits for state_mutex_. So the termination is still waited for
    // briefly before giving up. A forced stop is not tried, since no stop request reached the resource.
    if (!stopped.has_value() || (stopped.value() != RequestResult::kSuccess))
    {
        if (waitUntilTerminated(lock, kMaxSigKillDelay))
        {
            return true;
        }

        LM_LOG_ERROR() << ComponentId{config_->name, &handle} << "failed to request the resource to stop";
        state_ = TerminatedState{};
        return false;
    }

    if (waitUntilTerminated(lock, std::chrono::milliseconds{config_->deployment_config.shutdown_timeout_ms}))
    {
        return true;
    }

    LM_LOG_WARN() << ComponentId{config_->name, &handle}
                  << "did not terminate within the shutdown timeout, stopping it forcefully";
    const Result<RequestResult> forced = force_stop_action_->forceStop(cpp::stop_token{}, handle);

    // Like the stop request, a forced stop also fails if the resource has terminated in the meantime,
    // so the termination is waited for in either case.
    if (waitUntilTerminated(lock, kMaxSigKillDelay))
    {
        return true;
    }

    if (!forced.has_value() || (forced.value() != RequestResult::kSuccess))
    {
        LM_LOG_ERROR() << ComponentId{config_->name, &handle} << "failed to stop the resource forcefully";
    }
    else
    {
        LM_LOG_ERROR() << ComponentId{config_->name, &handle} << "did not terminate after being stopped forcefully";
    }

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
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        ready != nullptr, "Cannot deactivate component while it is activating or deactivating");
    const Handle handle = ready->handle_;

    // TODO: Failure to deactivate the supervision leaves the supervision expired
    // also on next restart. Failure must be escalated
    static_cast<void>(deactivateSupervision());

    return stopAndWait(lock, handle) ? RequestResult::kSuccess : RequestResult::kFailure;
}

IComponent::TerminationResult Component::tryHandleTermination(const Handle& terminated, int32_t status)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};
    return tryHandleTerminationInternally(terminated, status);
}

IComponent::TerminationResult Component::tryHandleTerminationInternally(const Handle& terminated, int32_t status)
{
    Handle* const handle = currentHandle();

    // The component already gave up on the resource, e.g. after a failed cleanup.
    if (handle == nullptr)
    {
        LM_LOG_DEBUG() << ComponentId{config_->name, &terminated}
                       << "ignoring termination, the resource is no longer tracked";
        return TerminationResult::kIgnore;
    }

    // The termination belongs to a resource the component gave up on, e.g. a previous instance which did
    // not terminate in time after being stopped forcefully, and has been started again since then.
    if (!isSameResource(*handle, terminated))
    {
        LM_LOG_DEBUG() << ComponentId{config_->name, &terminated}
                       << "ignoring termination of a previous instance of the resource";
        return TerminationResult::kIgnore;
    }

    // If we are waiting for termination, mark it as done. We do not care
    // about the exit code.
    if (std::holds_alternative<TerminatingState>(state_))
    {
        // req-id: comp_req__launch_man__process_log
        LM_LOG_INFO() << ComponentId{config_->name, &terminated} << "terminated after being stopped (expected)";
        state_ = TerminatedState{};
        // Notified while still locked: once stopAndWait() sees the termination, the component may be
        // destroyed, so this thread must not touch it after releasing the lock.
        termination_handled_.notify_all();
        return TerminationResult::kIgnore;
    }

    StartingState* const starting = std::get_if<StartingState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        (starting != nullptr) || (std::get_if<ReadyState>(&state_) != nullptr),
        "Cannot handle termination in this state");

    const bool terminationExpected = isExpectedTermination(status);
    const std::string_view phase = (starting != nullptr) ? "while starting" : "while ready";
    if (terminationExpected)
    {
        LM_LOG_DEBUG() << ComponentId{config_->name, &terminated} << "terminated" << phase << "(expected)";
    }
    else
    {
        LM_LOG_WARN() << ComponentId{config_->name, &terminated} << "terminated" << phase << "(unexpected)";
    }

    // Invalidate the resource to prevent acting on something which is already gone.
    invalidate(*handle);

    if (starting != nullptr)
    {
        // The ready condition decides whether its wait succeeds, the component decides whether the
        // termination is expected. activate() reports the outcome, so there is only one event.
        ready_condition_->handleTermination(*handle, status);
        starting->terminated_unexpectedly_ = !terminationExpected;
        return TerminationResult::kIgnore;
    }

    // TODO: Failure to deactivate the supervision leaves the supervision expired
    // also on next restart. Failure must be escalated
    static_cast<void>(deactivateSupervision());

    if (terminationExpected)
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

    LM_LOG_WARN() << ComponentId{config_->name, &ready->handle_}
                  << "alive supervision failed, stopping the resource forcefully";
    const Result<RequestResult> forced = force_stop_action_->forceStop(cpp::stop_token{}, ready->handle_);
    if (!forced.has_value() || (forced.value() != RequestResult::kSuccess))
    {
        // May also fail if the resource terminated in the meantime and its termination is not handled yet.
        LM_LOG_WARN() << ComponentId{config_->name, &ready->handle_}
                      << "failed to stop the resource forcefully after the supervision failure";
    }
}

Handle* Component::currentHandle()
{
    if (StartingState* const starting = std::get_if<StartingState>(&state_))
    {
        return &starting->handle_;
    }
    if (ReadyState* const ready = std::get_if<ReadyState>(&state_))
    {
        return &ready->handle_;
    }
    if (TerminatingState* const terminating = std::get_if<TerminatingState>(&state_))
    {
        return &terminating->handle_;
    }
    return nullptr;
}

void Component::forceStop()
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    const Handle* const handle = currentHandle();
    if ((handle != nullptr) && isAlive(*handle))
    {
        const Result<RequestResult> forced = force_stop_action_->forceStop(cpp::stop_token{}, *handle);
        if (!forced.has_value() || (forced.value() != RequestResult::kSuccess))
        {
            // May also fail if the resource terminated in the meantime and its termination is not handled yet.
            LM_LOG_WARN() << ComponentId{config_->name, handle} << "failed to stop the resource forcefully";
        }
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
        LM_LOG_ERROR() << ComponentId{config_->name}
                       << "failed to get timestamp to activate supervision:" << errno_message(errno);
        return std::nullopt;
    }

    if (!supervision_handle_->activateSupervision(timestamp))
    {
        LM_LOG_ERROR() << ComponentId{config_->name} << "failed to activate supervision";
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
        LM_LOG_ERROR() << ComponentId{config_->name}
                       << "failed to get timestamp to deactivate supervision:" << errno_message(errno);
        return false;
    }

    if (!supervision_handle_->deactivateSupervision(timestamp))
    {
        LM_LOG_ERROR() << ComponentId{config_->name} << "failed to deactivate supervision";
        return false;
    }

    return true;
}

}  // namespace score::mw::lifecycle::internal

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

// Using a struct for the parameters wouldn't make this easier to read.
// codeql[cpp/number-of-parameters-per-function] Dismiss
Component::Component(
    const IStartAction& start_action,
    const IStopAction& stop_action,
    const IForceStopAction& force_stop_action,
    const IReadyCondition& ready_condition,
    ISupervisionFactory& supervision_factory,
    configuration::ComponentConfig& config)
    : start_action_(start_action),
      stop_action_(stop_action),
      force_stop_action_(force_stop_action),
      ready_condition_(ready_condition),
      config_(config),
      supervision_handle_(constructSupervision(config, supervision_factory)),
      state_(TerminatedState{})
{
}

IComponent::RequestResult Component::activate(cpp::stop_token stop_token)
{
    std::unique_lock<std::mutex> lock{state_mutex_};

    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        std::holds_alternative<TerminatedState>(state_), "Cannot activate component in a running state");

    if (isSupervised(config_.component_properties.application_profile.application_type) && !supervision_handle_)
    {
        LM_LOG_ERROR() << config_.name << ": alive supervision is not set up";
        state_ = FaultState{};
        return RequestResult::kFailure;
    }

    const Result<Handle> start_result = start_action_.start(stop_token);
    if (!start_result.has_value())
    {
        state_ = FaultState{};
        return RequestResult::kFailure;
    }
    const Handle& handle = start_result.value();

    cpp::stop_source stop_activation{};
    const cpp::stop_callback forward_stop{stop_token, [&stop_activation] {
                                              // This can fail if stop is requested from both places.
                                              static_cast<void>(stop_activation.request_stop());
                                          }};

    state_ = StartingState{handle, stop_activation};

    // Wait for the ready condition. The state is unlocked so that `deactivate` or
    // `tryHandleTermination` are callable while we are waiting.
    lock.unlock();
    if (!ready_condition_.wait(stop_activation.get_token(), handle).has_value())
    {
        return RequestResult::kFailure;
    }
    lock.lock();

    // If `deactivate` or `tryHandleTermination` was called during the wait then
    // activation fails.
    if (stop_activation.stop_requested())
    {
        return RequestResult::kFailure;
    }

    // The state could change while the lock was released, so we have to check.
    if (const StartingState* const starting = std::get_if<StartingState>(&state_))
    {
        if (activateSupervision())
        {
            state_ = ReadyState{starting->handle_};
            return RequestResult::kSuccess;
        }
        else
        {
            state_ = FaultState{starting->handle_};
            return RequestResult::kFailure;
        }
    }

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("If stop was not requested, then state must still be StartingState");
}

IComponent::RequestResult Component::deactivate(cpp::stop_token stop_token)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    StartingState* const starting = std::get_if<StartingState>(&state_);
    const ReadyState* const ready = std::get_if<ReadyState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        (starting != nullptr) || (ready != nullptr), "Cannot deactivate component in a non-running state");
    const Handle& handle = (starting != nullptr) ? starting->handle_ : ready->handle_;

    if (!deactivateSupervision())
    {
        state_ = FaultState{handle};
        return RequestResult::kFailure;
    }

    // If the component is still waiting for ready conditions, cancel that.
    if (starting != nullptr)
    {
        static_cast<void>(starting->stop_activation_.request_stop());
    }

    const Result<RequestResult> result = stop_action_.stop(stop_token, handle);
    if (!result.has_value() || result.value() != RequestResult::kSuccess)
    {
        state_ = FaultState{handle};
        return RequestResult::kFailure;
    }

    // `tryHandleTermination` tells us when the termination is done.
    state_ = TerminatingState{handle};
    return RequestResult::kSuccess;
}

IComponent::TerminationResult Component::tryHandleTermination(int32_t status)
{
    const std::lock_guard<std::mutex> lock{state_mutex_};

    // If we are waiting for termination, mark it as done. We do not care
    // about the exit code.
    if (std::holds_alternative<TerminatingState>(state_))
    {
        state_ = TerminatedState{};
        return TerminationResult::kIgnore;
    }

    StartingState* const starting = std::get_if<StartingState>(&state_);
    ReadyState* const ready = std::get_if<ReadyState>(&state_);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        (starting != nullptr) || (ready != nullptr), "Cannot handle termination in a non-running state");
    Handle& handle = (starting != nullptr) ? starting->handle_ : ready->handle_;

    // Invalidate the resource to prevent acting on something which is already gone.
    invalidate(handle);

    if (ready != nullptr && !deactivateSupervision())
    {
        state_ = FaultState{handle};
        return TerminationResult::kFailure;
    }

    // Certain ready conditions can accept terminations.
    if (starting != nullptr && ready_condition_.tryHandleTermination(handle, status))
    {
        return TerminationResult::kIgnore;
    }

    if (config_.component_properties.application_profile.is_self_terminating && status == 0)
    {
        return TerminationResult::kIgnore;
    }

    // If none of the above cases apply, then termination is unexpected.
    if (starting != nullptr)
    {
        // Interrupt the worker thread waiting in activate().
        static_cast<void>(starting->stop_activation_.request_stop());
    }

    state_ = FaultState{};
    return TerminationResult::kFailure;
}

IdentifierHash Component::getIdentifier() const
{
    return IdentifierHash{config_.name};
}

bool Component::active() const
{
    const std::lock_guard<std::mutex> lock{state_mutex_};
    return std::holds_alternative<ReadyState>(state_);
}

bool Component::activateSupervision() const
{
    if (!supervision_handle_)
    {
        return true;
    }

    timespec timestamp{};

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
        LM_LOG_ERROR() << config_.name << ": failed to get timestamp to activate supervision:" << errno_message(errno);
        return false;
    }

    if (!supervision_handle_->activateSupervision(timestamp))
    {
        LM_LOG_ERROR() << config_.name << ": failed to activate supervision";
        return false;
    }

    return true;
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
        LM_LOG_ERROR() << config_.name
                       << ": failed to get timestamp to deactivate supervision: " << errno_message(errno);
        return false;
    }

    if (!supervision_handle_->deactivateSupervision(timestamp))
    {
        LM_LOG_ERROR() << config_.name << ": failed to deactivate supervision";
        return false;
    }

    return true;
}

}  // namespace score::mw::lifecycle::internal

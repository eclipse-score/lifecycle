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

namespace score::mw::lifecycle::internal
{

Component::Component(
    const IStartAction& start_action,
    const IStopAction& stop_action,
    const IForceStopAction& force_stop_action,
    const cpp::span<std::reference_wrapper<const IReadyCondition>> ready_conditions,
    bool self_terminating,
    IdentifierHash identifier)
    : start_action_(start_action),
      stop_action_(stop_action),
      force_stop_action_(force_stop_action),
      ready_conditions_(ready_conditions),
      self_terminating_(self_terminating),
      identifier_(identifier),
      state_(TerminatedState{})
{
}

IComponent::RequestResult Component::activate(cpp::stop_token stop_token)
{
    return std::visit(
        [&](auto state) {
            return state.activate(*this, stop_token);
        },
        state_);
}

IComponent::RequestResult Component::TerminatedState::activate(Component& component, cpp::stop_token stop_token)
{
    const Result<Handle> start_result = component.start_action_.start(stop_token);
    if (!start_result.has_value())
    {
        return cpp::make_unexpected(ComponentError::kErrorBeforeReady);
    }
    const Handle handle = start_result.value();

    component.state_ = StartingState{handle};

    for (const IReadyCondition& ready_condition : component.ready_conditions_)
    {
        if (!ready_condition.wait(stop_token, handle).has_value())
        {
            return cpp::make_unexpected(ComponentError::kErrorBeforeReady);
        }
    }

    component.state_ = ReadyState{handle};

    return RequestState::kSuccess;
}

IComponent::RequestResult Component::StartingState::activate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot activate component in StartingState");
}

IComponent::RequestResult Component::ReadyState::activate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot activate component in ReadyState");
}

IComponent::RequestResult Component::TerminatingState::activate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot activate component in TerminatingState");
}

IComponent::RequestResult Component::FaultState::activate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot activate component in FaultState");
}

IComponent::RequestResult Component::deactivate(cpp::stop_token stop_token)
{
    return std::visit(
        [&](auto state) {
            return state.deactivate(*this, stop_token);
        },
        state_);
}

IComponent::RequestResult Component::TerminatedState::deactivate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot deactivate component in TerminatedState");
}

IComponent::RequestResult Component::StartingState::deactivate(Component& component, cpp::stop_token stop_token)
{
    if (!component.stop_action_.stop(stop_token, handle_).has_value())
    {
        return cpp::make_unexpected(ComponentError::kErrorAfterReady);
    }

    component.state_ = TerminatingState{handle_};
    return RequestState::kWaiting;
}

IComponent::RequestResult Component::ReadyState::deactivate(Component& component, cpp::stop_token stop_token)
{
    if (!component.stop_action_.stop(stop_token, handle_).has_value())
    {
        return cpp::make_unexpected(ComponentError::kErrorAfterReady);
    }

    component.state_ = TerminatingState{handle_};
    return RequestState::kWaiting;
}

IComponent::RequestResult Component::TerminatingState::deactivate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot deactivate component in TerminatingState");
}

IComponent::RequestResult Component::FaultState::deactivate(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] cpp::stop_token stop_token)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot deactivate component in FaultState");
}

IComponent::RequestResult Component::tryHandleTermination(int32_t status)
{
    return std::visit(
        [&](auto state) {
            return state.tryHandleTermination(*this, status);
        },
        state_);
}

IComponent::RequestResult Component::TerminatedState::tryHandleTermination(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] int32_t status)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot handle termination for a component in TerminatedState");
}

IComponent::RequestResult Component::StartingState::tryHandleTermination(Component& component, int32_t status)
{
    for (const IReadyCondition& ready_condition : component.ready_conditions_)
    {
        if (ready_condition.tryHandleTermination(status))
        {
            return RequestState::kSuccess;
        }
    }

    // TODO: We need to interrupt the worker thread which is sitting in the activate() method
    //       (and prevent it from changing the state to successful)

    component.state_ = FaultState{};
    return cpp::make_unexpected(ComponentError::kErrorAfterReady);
}

IComponent::RequestResult Component::ReadyState::tryHandleTermination(Component& component, int32_t status)
{
    if (component.self_terminating_ && status == 0)
    {
        component.state_ = TerminatedState{};
        return RequestState::kSuccess;
    }

    component.state_ = FaultState{};
    return cpp::make_unexpected(ComponentError::kErrorAfterReady);
}

IComponent::RequestResult Component::TerminatingState::tryHandleTermination(
    Component& component,
    [[maybe_unused]] int32_t status)
{
    // Since we requested termination, we do not care about the exit code.
    component.state_ = TerminatedState{};
    return RequestState::kSuccess;
}

IComponent::RequestResult Component::FaultState::tryHandleTermination(
    [[maybe_unused]] Component& component,
    [[maybe_unused]] int32_t status)
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("Cannot handle termination for a component in FaultState");
}

IdentifierHash Component::getIdentifier() const
{
    return identifier_;
}

bool Component::active() const
{
    return std::holds_alternative<ReadyState>(state_);
}

}  // namespace score::mw::lifecycle::internal

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
    const IStartAction* start_action,
    const IStopAction* stop_action,
    const IForceStopAction* force_stop_action,
    const std::vector<const IReadyCondition*> ready_conditions,
    IdentifierHash identifier)
    : handle_(EmptyHandle{}),
      start_action_(start_action),
      stop_action_(stop_action),
      force_stop_action_(force_stop_action),
      ready_conditions_(ready_conditions),
      identifier_(identifier)
{
}

IComponent::RequestResult Component::activate(score::cpp::stop_token stop_token)
{
    handle_ = start_action_->start();
    for (const IReadyCondition* ready_condition : ready_conditions_)
    {
        ready_condition->wait(handle_.value());
    }
    return RequestState::kSuccess;
}

IComponent::RequestResult Component::deactivate(score::cpp::stop_token stop_token)
{
    if (handle_.has_value())
    {
        stop_action_->stop(handle_.value());
    }
    handle_ = std::nullopt;
    return RequestState::kSuccess;
}

IComponent::RequestResult Component::tryHandleTermination(int32_t status)
{
    return RequestState::kWaiting;
}

IdentifierHash Component::getIdentifier() const
{
    return identifier_;
}

bool Component::active() const
{
    return handle_.has_value();
}

}  // namespace score::mw::lifecycle::internal

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

#include "termination_waiter.hpp"

namespace score::mw::lifecycle::internal
{

TerminationWaiter::SharedState::SharedState()
    : state_mutex(), promise(), shared_future(promise.GetInterruptibleFuture().value().Share()), is_terminated(false)
{
}

TerminationWaiter::TerminationWaiter() : state_(std::make_shared<SharedState>())
{
}

TerminationWaiter::~TerminationWaiter() = default;

void TerminationWaiter::terminated() noexcept(false)
{
    if (!state_)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(state_->state_mutex);
    if (!state_->is_terminated)
    {
        state_->is_terminated = true;
        state_->promise.SetValue();
    }
}

bool TerminationWaiter::wait_with_timeout(
    const score::cpp::stop_token& stop_token,
    const std::chrono::milliseconds& rel_timeout_ms)
{
    auto status = state_->shared_future.WaitFor(stop_token, rel_timeout_ms);

    return (status.has_value());
}

}  // namespace score::mw::lifecycle::internal

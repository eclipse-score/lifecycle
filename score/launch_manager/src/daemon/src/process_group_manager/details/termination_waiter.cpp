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

TerminationWaiter::TerminationWaiter()
{
}

TerminationWaiter::~TerminationWaiter() = default;

void TerminationWaiter::terminated() noexcept(false)
{
    std::lock_guard<std::mutex> lock(this->terminated_mutex_);
    if (!this->is_terminated_)
    {
        this->is_terminated_ = true;
        this->terminated_cv_.notify_all();
    }
}

bool TerminationWaiter::wait_with_timeout(
    const score::cpp::stop_token& stop_token,
    const std::chrono::milliseconds& rel_timeout_ms)
{
    std::unique_lock<std::mutex> lock(this->terminated_mutex_);
    return this->terminated_cv_.wait_for(lock, stop_token, rel_timeout_ms, [this] {
        return this->is_terminated_;
    });
}

}  // namespace score::mw::lifecycle::internal

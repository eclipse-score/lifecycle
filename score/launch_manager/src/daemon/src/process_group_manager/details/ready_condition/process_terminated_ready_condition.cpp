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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/process_terminated_ready_condition.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <score/assert.hpp>
#include <variant>

namespace score::mw::lifecycle::internal
{

ProcessTerminatedReadyCondition::ProcessTerminatedReadyCondition(std::chrono::milliseconds timeout) : timeout_(timeout)
{
}

void ProcessTerminatedReadyCondition::prepare() const
{
    /// A termination of the previous process may still be reported after its wait
    /// has failed. Without this reset, that stale notification would satisfy the
    /// next wait before the new process has terminated.
    waiter_.reset();
}

Result<void> ProcessTerminatedReadyCondition::wait(cpp::stop_token stop_token, const Handle& handle) const
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        std::holds_alternative<ProcessHandle>(handle),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");

    if (!waiter_.wait_with_timeout(stop_token, timeout_))
    {
        // The wait also ends without a termination if a stop was requested, which is not a timeout.
        return MakeUnexpected(stop_token.stop_requested() ? ExecErrc::kGeneralError : ExecErrc::kTimeout);
    }

    // Only a successful termination satisfies the ready condition.
    if (status_.load() != 0)
    {
        return MakeUnexpected(ExecErrc::kGeneralError);
    }

    return {};
}

void ProcessTerminatedReadyCondition::handleTermination(const Handle& handle, int32_t status) const
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        std::holds_alternative<ProcessHandle>(handle),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");

    // Store the status before waking up the waiter, so that wait() sees it once it is woken up.
    // Every termination wakes up the waiter: a failed termination must end the wait as well.
    status_.store(status);
    waiter_.terminated();
}

}  // namespace score::mw::lifecycle::internal

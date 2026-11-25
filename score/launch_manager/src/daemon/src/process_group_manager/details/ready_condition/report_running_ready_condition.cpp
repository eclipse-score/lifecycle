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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <score/assert.hpp>
#include <score/stop_token.hpp>
#include <memory>
#include <variant>

namespace score::mw::lifecycle::internal
{

ReportRunningReadyCondition::ReportRunningReadyCondition(osal::IProcess& launcher) : launcher_(launcher)
{
}

Result<void> ReportRunningReadyCondition::wait(
    cpp::stop_token stop_token,
    const Handle& handle,
    std::chrono::milliseconds timeout) const
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process != nullptr, "ReportRunningReadyCondition should only be called with a ProcessHandle");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process->sync != nullptr,
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");

    // A stop request wakes up the waiting thread with a post, like a termination does. The callback
    // captures the shared pointer by value, so the shared memory stays alive while it runs. If the
    // token is already stopped, the callback runs immediately.
    const cpp::stop_callback wake_on_stop{stop_token, [sync = process->sync] {
                                              static_cast<void>(sync->send_sync_.post());
                                          }};

    // waitForkRunning only succeeds if the process itself reported kRunning, not if it was woken up by
    // handleTermination or a stop request.
    const osal::OsalReturnType result = launcher_.waitForkRunning(process->sync, timeout);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return {};
    }

    if (result == osal::OsalReturnType::kTimeout)
    {
        return MakeUnexpected(ExecErrc::kTimeout);
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

void ReportRunningReadyCondition::handleTermination(const Handle& handle, int32_t /*status*/) const
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process != nullptr, "ReportRunningReadyCondition should only be called with a ProcessHandle");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process->sync != nullptr,
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");

    // If posting fails, the ready condition will wait until its timeout is over
    static_cast<void>(process->sync->send_sync_.post());
}

}  // namespace score::mw::lifecycle::internal

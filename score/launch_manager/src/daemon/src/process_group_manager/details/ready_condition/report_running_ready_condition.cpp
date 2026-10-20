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
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

ReportRunningReadyCondition::ReportRunningReadyCondition(
    osal::IProcess& launcher,
    std::optional<std::chrono::milliseconds> timeout)
    : launcher_(launcher), timeout_(timeout)
{
}

Result<void> ReportRunningReadyCondition::wait(cpp::stop_token stop_token, const Handle& handle) const
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process != nullptr, "ReportRunningReadyCondition should only be called with a ProcessHandle");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process->sync != nullptr,
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");

    const osal::OsalReturnType result = launcher_.waitForkRunning(process->sync, timeout_);

    // If the PID is invalid, the process died and the notification was sent
    // from tryHandleTermination rather than the process itself.
    if (result == osal::OsalReturnType::kSuccess && process->pid > 0)
    {
        return {};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

bool ReportRunningReadyCondition::tryHandleTermination(const Handle& handle, int32_t status) const
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process != nullptr, "ReportRunningReadyCondition should only be called with a ProcessHandle");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process->sync != nullptr,
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");

    // FIXME: This static_cast was copied from the old code at
    // https://github.com/eclipse-score/lifecycle/blob/876c5df184ee0008017334303c12f1c2ee75e42e/score/launch_manager/src/daemon/src/process_group_manager/details/process_info_node.cpp#L171
    // Should errors be handled here?
    static_cast<void>(process->sync->send_sync_.post());

    return true;
}

}  // namespace score::mw::lifecycle::internal

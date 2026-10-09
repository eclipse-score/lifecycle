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

Result<void> ReportRunningReadyCondition::wait(cpp::stop_token stop_token, const Handle handle) const
{
    return std::visit(
        [this](auto&& handle) {
            return visitWait(handle);
        },
        handle);
}

Result<void> ReportRunningReadyCondition::visitWait(const ProcessHandle process) const
{
    const osal::OsalReturnType result = launcher_.waitForkRunning(process.sync, timeout_);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return {};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

Result<void> ReportRunningReadyCondition::visitWait(const Handle handle) const
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE(
        "ReportRunningReadyCondition should only be called with a ProcessHandle");
}

}  // namespace score::mw::lifecycle::internal

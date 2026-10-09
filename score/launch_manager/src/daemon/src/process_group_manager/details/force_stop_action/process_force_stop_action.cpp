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

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/process_force_stop_action.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

ProcessForceStopAction::ProcessForceStopAction(osal::IProcess& launcher) : launcher_(launcher)
{
}

Result<void> ProcessForceStopAction::forceStop(cpp::stop_token stop_token, const Handle handle) const
{
    return std::visit(
        [this](auto&& handle) {
            return visitForceStop(handle);
        },
        handle);
}

Result<void> ProcessForceStopAction::visitForceStop(const ProcessHandle process) const
{
    if (process.pid == -1)
    {
        return {};
    }

    const osal::OsalReturnType result = launcher_.forceTermination(process.pid);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return {};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

Result<void> ProcessForceStopAction::visitForceStop(const Handle handle) const
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("ProcessForceStopAction should only be called with a ProcessHandle");
}

}  // namespace score::mw::lifecycle::internal

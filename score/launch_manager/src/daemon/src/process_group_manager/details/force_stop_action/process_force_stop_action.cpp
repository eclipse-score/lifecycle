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
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

ProcessForceStopAction::ProcessForceStopAction(osal::ProcessLauncher& launcher) : launcher_(launcher)
{
}

Result<void> ProcessForceStopAction::force_stop(const Handle handle) const
{
    return std::visit(*this, handle);
}

Result<void> ProcessForceStopAction::operator()(const ProcessHandle process) const
{
    const osal::OsalReturnType result = launcher_.forceTermination(process.pid);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return {};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

Result<void> ProcessForceStopAction::operator()(const Handle handle) const
{
    return MakeUnexpected(ExecErrc::kNotImplemented);
}

}  // namespace score::mw::lifecycle::internal

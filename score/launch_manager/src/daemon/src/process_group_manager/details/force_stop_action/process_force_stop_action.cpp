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

Result<IComponent::RequestResult> ProcessForceStopAction::forceStop(
    cpp::stop_token /*stop_token*/,
    const Handle& handle) const
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        process != nullptr, "ProcessForceStopAction should only be called with a ProcessHandle");

    if (process->pid == -1)
    {
        return IComponent::RequestResult::kSuccess;
    }

    const osal::OsalReturnType result = launcher_.forceTermination(process->pid);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return IComponent::RequestResult::kSuccess;
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

}  // namespace score::mw::lifecycle::internal

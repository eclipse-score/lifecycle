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

#include "score/mw/launch_manager/process_group_manager/details/stop_action/process_stop_action.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

ProcessStopAction::ProcessStopAction(osal::IProcess& launcher) : launcher_(launcher)
{
}

Result<IComponent::RequestState> ProcessStopAction::stop(cpp::stop_token stop_token, const Handle handle) const
{
    return std::visit(
        [this](auto&& handle) {
            return visitStop(handle);
        },
        handle);
}

Result<IComponent::RequestState> ProcessStopAction::visitStop(const ProcessHandle process) const
{
    if (process.pid == -1)
    {
        return IComponent::RequestState::kSuccess;
    }

    const osal::OsalReturnType result = launcher_.requestTermination(process.pid);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return IComponent::RequestState::kWaiting;
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

Result<IComponent::RequestState> ProcessStopAction::visitStop(const Handle handle) const
{
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("ProcessStopAction should only be called with a ProcessHandle");
}

}  // namespace score::mw::lifecycle::internal

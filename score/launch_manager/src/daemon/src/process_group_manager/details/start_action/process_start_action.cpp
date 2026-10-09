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

#include "score/mw/launch_manager/process_group_manager/details/start_action/process_start_action.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

ProcessStartAction::ProcessStartAction(osal::IProcess& launcher, const configuration::ComponentConfig& config)
    : launcher_(launcher), config_(config)
{
}

Result<Handle> ProcessStartAction::start(cpp::stop_token stop_token) const
{
    osal::ProcessID pid = 0;
    osal::IpcCommsP sync = nullptr;

    const auto result = launcher_.startProcess(pid, sync, config_);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return ProcessHandle{pid, sync};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

}  // namespace score::mw::lifecycle::internal

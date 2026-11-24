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
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <score/stop_token.hpp>
#include <string>

namespace score::mw::lifecycle::internal
{

ProcessStartAction::ProcessStartAction(
    osal::IProcess& launcher,
    SafeProcessMapInserter& process_map,
    const configuration::ComponentConfig& config)
    : launcher_(launcher), process_map_(process_map), config_(config)
{
}

Result<StartedResource> ProcessStartAction::start(cpp::stop_token /*stop_token*/, IComponent& component) const
{
    osal::ProcessID pid = 0;
    osal::IpcCommsP sync = nullptr;

    if (launcher_.startProcess(pid, sync, config_) != osal::OsalReturnType::kSuccess)
    {
        return MakeUnexpected(ExecErrc::kGeneralError);
    }

    const std::optional<StartedResource> started = track(pid, sync, component);
    if (started.has_value())
    {
        return started.value();
    }

    // The termination of an untracked process would never be reported, so do not leave it running.
    LM_LOG_ERROR() << config_.name << ": could not add pid" << pid << "to the process map, killing the process";
    static_cast<void>(launcher_.forceTermination(pid));
    return MakeUnexpected(ExecErrc::kGeneralError);
}

std::optional<StartedResource>
ProcessStartAction::track(osal::ProcessID pid, const osal::IpcCommsP& sync, IComponent& component) const
{
    const SafeProcessMapInsertResult tracked = process_map_.insertIfNotTerminated(pid, &component);
    switch (tracked.type)
    {
        case SafeProcessMapReturnType::kInserted:
            // The process is still running, its termination is reported later.
            return StartedResource{ProcessHandle{pid, sync}, std::nullopt};

        case SafeProcessMapReturnType::kMatched:
            // The process has already terminated, so its termination is not reported later.
            return StartedResource{ProcessHandle{pid, sync}, tracked.exit_status};

        case SafeProcessMapReturnType::kInsertionError:
        case SafeProcessMapReturnType::kInvalidIdError:
            break;
    }

    return std::nullopt;
}

}  // namespace score::mw::lifecycle::internal

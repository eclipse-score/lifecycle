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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

FileReadyCondition::FileReadyCondition(
    const osal::IFileWaiter& file_waiter,
    score::safecpp::zstring_view file_path,
    configuration::FileExistenceState state,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds interval)
    : file_waiter_(file_waiter), file_path_(file_path), state_(state), timeout_(timeout), interval_(interval)
{
}

Result<void> FileReadyCondition::wait(cpp::stop_token stop_token, [[maybe_unused]] const Handle) const
{
    const osal::OsalReturnType result = file_waiter_.waitForFile(file_path_, state_, timeout_, interval_, stop_token);

    if (result == osal::OsalReturnType::kSuccess)
    {
        return {};
    }

    return MakeUnexpected(ExecErrc::kGeneralError);
}

}  // namespace score::mw::lifecycle::internal

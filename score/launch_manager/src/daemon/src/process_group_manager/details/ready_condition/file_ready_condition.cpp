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
#include <score/stop_token.hpp>

namespace score::mw::lifecycle::internal
{

FileReadyCondition::FileReadyCondition(
    const osal::IFileWaiter& file_waiter,
    score::safecpp::zstring_view file_path,
    configuration::FileExistenceState state,
    std::chrono::milliseconds interval,
    bool is_self_terminating)
    : file_waiter_(file_waiter),
      file_path_(file_path),
      state_(state),
      interval_(interval),
      is_self_terminating_(is_self_terminating)
{
}

void FileReadyCondition::prepare() const
{
    // A termination of the previous process may still be reported after its wait has failed.
    terminated_ = cpp::stop_source{};
}

Result<void> FileReadyCondition::wait(
    cpp::stop_token stop_token,
    [[maybe_unused]] const Handle&,
    std::chrono::milliseconds timeout) const
{
    // The file wait is ended by a stop request of the caller or by the termination of the resource.
    cpp::stop_source wait_stop;
    const cpp::stop_token terminated = terminated_.get_token();
    const cpp::stop_callback stop_on_request{stop_token, [&wait_stop] {
                                                 static_cast<void>(wait_stop.request_stop());
                                             }};
    const cpp::stop_callback stop_on_termination{terminated, [&wait_stop] {
                                                     static_cast<void>(wait_stop.request_stop());
                                                 }};

    osal::OsalReturnType result =
        file_waiter_.waitForFile(file_path_, state_, timeout, interval_, wait_stop.get_token());

    if ((result != osal::OsalReturnType::kSuccess) && (terminated.stop_requested() || stop_token.stop_requested()))
    {
        // The wait was ended early, possibly before the file was checked at all. Check once more.
        result =
            file_waiter_.waitForFile(file_path_, state_, std::chrono::milliseconds{0}, interval_, cpp::stop_token{});
        if (result == osal::OsalReturnType::kSuccess)
        {
            return {};
        }
        return MakeUnexpected(ExecErrc::kGeneralError);
    }

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

void FileReadyCondition::handleTermination(const Handle& /*handle*/, int32_t status) const
{
    // An expected exit does not prevent the file from reaching its state, so the wait continues.
    if (is_self_terminating_ && (status == 0))
    {
        return;
    }
    static_cast<void>(terminated_.request_stop());
}

}  // namespace score::mw::lifecycle::internal

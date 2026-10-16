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

#ifndef SCORE_LCM_FILE_READY_CONDITION_HPP_INCLUDED
#define SCORE_LCM_FILE_READY_CONDITION_HPP_INCLUDED

#include "score/language/safecpp/string_view/zstring_view.h"
#include "score/mw/launch_manager/osal/ifile_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/result/result.h"
#include <chrono>

namespace score::mw::lifecycle::internal
{

/// @brief A ready condition which waits for a file to exist/not exist.
/// @details This may be useful for components which mount filesystems,
///          or set up devices under `/dev`.
class FileReadyCondition final : public IReadyCondition
{
  public:
    /// @brief Creates a new file ready condition.
    /// @param file_waiter The waiter used to check the state of the file.
    /// @param file_path The path to the file to wait for.
    /// @param state The desired state of the file.
    /// @param timeout The maximum duration to wait for the file condition.
    /// @param interval How often to poll for the file.
    explicit FileReadyCondition(
        const osal::IFileWaiter& file_waiter,
        score::safecpp::zstring_view file_path,
        configuration::FileExistenceState state,
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds interval);

    /// @brief Wait until the resource represented by the given handle is ready.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @return Whether the wait was successful, or failed with an error.
    Result<void> wait(cpp::stop_token stop_token, const Handle handle) const override;

  private:
    /// @brief The waiter used to check file state.
    const osal::IFileWaiter& file_waiter_;

    /// @brief The path to the file to wait for.
    const score::safecpp::zstring_view file_path_;

    /// @brief The desired state of the file.
    const configuration::FileExistenceState state_;

    /// @brief The maximum duration to wait for the file to exist/not exist.
    const std::chrono::milliseconds timeout_;

    /// @brief How often to poll for the file.
    const std::chrono::milliseconds interval_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_FILE_READY_CONDITION_HPP_INCLUDED

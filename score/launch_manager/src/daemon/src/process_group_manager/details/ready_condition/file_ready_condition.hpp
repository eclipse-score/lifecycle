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
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ifile_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/result/result.h"
#include <score/stop_token.hpp>
#include <chrono>
#include <cstdint>

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
    /// @param interval How often to poll for the file.
    explicit FileReadyCondition(
        const osal::IFileWaiter& file_waiter,
        score::safecpp::zstring_view file_path,
        configuration::FileExistenceState state,
        std::chrono::milliseconds interval);

    /// @brief Prepare the ready condition for a new activation.
    /// @details Discards a termination left over from a previous activation.
    void prepare() const override;

    /// @brief Wait until the file has reached the desired state.
    /// @details A reported termination ends the wait. The file is then checked once more, because the
    ///          process may have changed it just before it exited. A stop request on the token ends the
    ///          wait without this final check.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @param timeout The maximum duration to wait for the file condition.
    /// @return Whether the wait was successful, or failed with an error.
    Result<void> wait(cpp::stop_token stop_token, const Handle& handle, std::chrono::milliseconds timeout)
        const override;

    /// @brief Notify the ready condition that the component has already terminated.
    /// @details Ends a running wait(), see wait().
    /// @param handle The component that terminated.
    /// @param status Exit code of the component.
    void handleTermination(const Handle& handle, int32_t status) const override;

  private:
    /// @brief The waiter used to check file state.
    const osal::IFileWaiter& file_waiter_;

    /// @brief The path to the file to wait for.
    const score::safecpp::zstring_view file_path_;

    /// @brief The desired state of the file.
    const configuration::FileExistenceState state_;

    /// @brief How often to poll for the file.
    const std::chrono::milliseconds interval_;

    /// @brief Stopped once the resource of the current activation has terminated.
    /// @note Mutable because the IReadyCondition API is const. It is only replaced in prepare(), while
    ///       neither wait() nor handleTermination() can run.
    mutable cpp::stop_source terminated_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_FILE_READY_CONDITION_HPP_INCLUDED

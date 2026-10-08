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

#ifndef SCORE_LCM_REPORT_RUNNING_HPP_INCLUDED
#define SCORE_LCM_REPORT_RUNNING_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"
#include <chrono>
#include <optional>

namespace score::mw::lifecycle::internal
{

/// @brief A ready condition which waits for a process to report running
///        through a shared memory channel.
class ReportRunningReadyCondition final : public IReadyCondition
{
  public:
    /// @brief Creates a new report running ready condition.
    /// @param launcher The process launcher used to wait for the process.
    /// @param timeout The maximum duration to wait for the process to report running,
    ///                or no timeout if not specified.
    explicit ReportRunningReadyCondition(osal::IProcess& launcher, std::optional<std::chrono::milliseconds> timeout);

    /// @brief Wait until the resource represented by the given handle is ready.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @return Whether the wait was successful, or failed with an error.
    Result<void> wait(cpp::stop_token stop_token, const Handle handle) const override;

  private:
    /// @brief Visitor method for process handles.
    Result<void> visitWait(const ProcessHandle process) const;

    /// @brief Visitor method for unsupported handles.
    Result<void> visitWait(const Handle handle) const;

    /// @brief The process launcher used to wait for the process.
    osal::IProcess& launcher_;

    /// @brief The maximum duration to wait for the process to report running,
    ///        or no timeout if not specified.
    const std::optional<std::chrono::milliseconds> timeout_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_REPORT_RUNNING_HPP_INCLUDED

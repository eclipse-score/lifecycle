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

#ifndef SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED
#define SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/iforce_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

/// @brief A force stop action which forcefully stops a POSIX process.
/// @details The process is stopped by sending SIGKILL, which cannot be caught
///          by the process and results in immediate termination.
class ProcessForceStopAction final : public IForceStopAction
{
  public:
    /// @brief Creates a new process force stop action.
    /// @param launcher The process launcher used to stop the process.
    explicit ProcessForceStopAction(osal::IProcess& launcher);

    /// @brief Forcefully stop the process represented by the given handle.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @param handle The resource to act upon.
    /// @return Whether the action was successful, or failed with an error.
    Result<void> forceStop(cpp::stop_token stop_token, const Handle handle) const override;

  private:
    /// @brief Visitor method for process handles.
    Result<void> visitForceStop(const ProcessHandle process) const;

    /// @brief Visitor method for unsupported handles.
    Result<void> visitForceStop(const Handle handle) const;

    /// @brief The process launcher used to stop the process.
    osal::IProcess& launcher_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED

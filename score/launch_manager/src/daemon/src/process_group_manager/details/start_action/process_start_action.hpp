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

#ifndef SCORE_LCM_PROCESS_START_ACTION_HPP_INCLUDED
#define SCORE_LCM_PROCESS_START_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

/// @brief A start action which starts a POSIX process.
class ProcessStartAction final : public IStartAction
{
  public:
    /// @brief Creates a new process start action.
    /// @param launcher The process launcher used to start the process.
    /// @param config Configuration describing the executable to launch
    ///               and its environment.
    ProcessStartAction(osal::IProcess& launcher, const configuration::ComponentConfig& config);

    /// @brief Start a process.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @return The handle of the started process, or an error if the action failed.
    Result<Handle> start(cpp::stop_token stop_token) const override;

  private:
    /// @brief The process launcher used to start the process.
    osal::IProcess& launcher_;

    /// @brief Configuration describing the executable to launch and its environment.
    const configuration::ComponentConfig& config_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_START_ACTION_HPP_INCLUDED

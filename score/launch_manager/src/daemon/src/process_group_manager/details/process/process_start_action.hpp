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
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/icomponent.hpp"
#include "score/result/result.h"
#include <optional>

namespace score::mw::lifecycle::internal
{

/// @brief A start action which starts a POSIX process.
class ProcessStartAction final : public IStartAction
{
  public:
    /// @brief Creates a new process start action.
    /// @param launcher The process launcher used to start the process.
    /// @param process_map Used to report the termination of the started process to its component.
    /// @param config Configuration describing the executable to launch
    ///               and its environment.
    ProcessStartAction(
        osal::IProcess& launcher,
        SafeProcessMapInserter& process_map,
        const configuration::ComponentConfig& config);

    /// @brief Start a process and insert it into the process map, so that its termination is
    ///        reported to @p component.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @param component The component which owns the process.
    /// @return The started process, with its exit status if it terminated before it could be
    ///         tracked, or an error if the process could not be started or tracked. A process which
    ///         could not be tracked is killed.
    Result<StartedResource> start(cpp::stop_token stop_token, IComponent& component) const override;

  private:
    /// @brief Insert the started process into the process map, so that its termination is
    ///        reported to @p component.
    /// @param pid The process ID of the started process.
    /// @param sync The synchronization object of the started process.
    /// @param component The component which owns the process.
    /// @return The started process, with its exit status if it has already terminated, or
    ///         std::nullopt if the process could not be inserted into the process map.
    [[nodiscard]] std::optional<StartedResource>
    track(osal::ProcessID pid, const osal::IpcCommsP& sync, IComponent& component) const;

    /// @brief The process launcher used to start the process.
    osal::IProcess& launcher_;

    /// @brief Used to report the termination of the started process to its component.
    SafeProcessMapInserter& process_map_;

    /// @brief Configuration describing the executable to launch and its environment.
    const configuration::ComponentConfig& config_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_START_ACTION_HPP_INCLUDED

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

#ifndef OSAL_START_PROCESS_HPP_INCLUDED
#define OSAL_START_PROCESS_HPP_INCLUDED

#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"

namespace score::mw::lifecycle::internal::osal
{

/// @brief Creates a new process executing the configured executable with the configured sandbox applied.
/// @details The process is started with the arguments, environment, working directory, scheduling parameters,
/// credentials, supplementary groups, resource limits, CPU affinity and security policy from @p config.
/// @param[out] pid Process ID of the new process.
/// @param[in] config Configuration of the process to start.
/// @param[in] comms_fd File descriptor of the shared memory segment used for communication. Ignored if @p comms
/// is null.
/// @param[in] comms Shared memory block used for communication, or null for processes that do not communicate.
/// @return kSuccess if a process was created, kFail otherwise. A process that was created but fails to apply its
/// configuration terminates with EXIT_FAILURE or SIGKILL, which is reported via the normal termination path.
OsalReturnType
startProcess(ProcessID& pid, const configuration::ComponentConfig& config, int comms_fd, const IpcCommsP& comms);

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_START_PROCESS_HPP_INCLUDED

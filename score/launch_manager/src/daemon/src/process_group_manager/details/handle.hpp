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

#ifndef SCORE_LCM_HANDLE_HPP_INCLUDED
#define SCORE_LCM_HANDLE_HPP_INCLUDED

#include <sys/types.h>
#include <variant>

#include "score/mw/launch_manager/osal/ipc_comms.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief Signifies that we are not managing any resource.
/// @details This is useful for run targets and synchronisation points,
///          which exist only to depend on other components.
struct EmptyHandle
{
};

/// @brief Represents a POSIX process that we are managing.
struct ProcessHandle
{
    pid_t pid;
    osal::IpcCommsP sync;
};

/// @brief Represents some resource that we are managing.
using Handle = std::variant<EmptyHandle, ProcessHandle>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_HANDLE_HPP_INCLUDED

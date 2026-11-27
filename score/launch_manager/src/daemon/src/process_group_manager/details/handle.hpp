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
#include <ostream>
#include <variant>

#include "score/mw/launch_manager/common/log.hpp"
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

/// @brief Changes a handle to indicate that the resource it pointed to is gone.
/// @param handle The handle to invalidate.
void invalidate(Handle& handle);

/// @brief Checks whether a handle still refers to a resource which can terminate.
/// @param handle The handle to check.
/// @return False for an EmptyHandle and for a handle which was invalidated, true otherwise.
bool isAlive(const Handle& handle);

/// @brief Checks whether two handles refer to the same resource.
/// @details Only the identity of the resource is compared (e.g. the pid of a process).
/// An EmptyHandle or an invalidated handle never
/// refers to the same resource as any other handle, since it does not refer to a resource at all.
/// @param lhs The first handle to compare.
/// @param rhs The second handle to compare.
/// @return True if both handles refer to the same resource.
bool isSameResource(const Handle& lhs, const Handle& rhs);

/// @brief Writes a description of the resource a handle refers to, for logging.
/// @details A ProcessHandle is written as "pid: <pid>". Nothing is written for an EmptyHandle or a
///          handle which was invalidated, since they do not refer to a resource.
/// @param stream The stream to write to.
/// @param handle The handle to describe.
/// @return The stream.
std::ostream& operator<<(std::ostream& stream, const Handle& handle);

#ifdef LC_LOG_SCORE_MW_LOG
/// @brief Writes a description of the resource a handle refers to directly to a log stream.
/// @details Same as the std::ostream overload, mw::log separates the arguments with a space itself.
/// @param stream The log stream to write to.
/// @param handle The handle to describe.
/// @return The log stream.
score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const Handle& handle);
#endif  // LC_LOG_SCORE_MW_LOG

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_HANDLE_HPP_INCLUDED

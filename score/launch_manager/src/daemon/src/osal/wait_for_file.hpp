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

#ifndef OSAL_WAIT_FOR_FILE_HPP_INCLUDED
#define OSAL_WAIT_FOR_FILE_HPP_INCLUDED

#include <chrono>
#include <string_view>

#include "return_types.hpp"

namespace score
{

namespace mw::lifecycle
{

namespace internal
{

namespace osal
{

/// @brief Default interval between two consecutive `stat` calls performed by wait_for_file().
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
constexpr std::chrono::milliseconds kDefaultFilePollInterval{2U};

/// @brief Block until the given path exists or the timeout expires.
///
/// The path is polled with `stat` because neither Linux nor QNX offer a portable way of waiting on the creation of a
/// single path. A monotonic clock (`std::chrono::steady_clock`) is used for the deadline so that a change of the
/// system clock by another thread cannot extend or shorten the wait.
///          - `stat` returns 0 if the path could be resolved, meaning the file exists.
///          - `stat` returns -1 with `errno` set to ENOENT/ENOTDIR while the path (or one of its parent directories)
///            does not exist yet, which is the case this function waits for.
///          - Any other `errno` (e.g. EACCES, ELOOP) is a permanent error and aborts the wait.
///            - https://pubs.opengroup.org/onlinepubs/9699919799/functions/stat.html
///
/// The path is checked once before the first sleep, so a timeout of zero performs a single existence check.
/// No distinction is made between file types: a directory, a FIFO or a socket at @p path also counts as existing.
///
/// @param timeout The maximum time to wait for the path to appear.
/// @param path The path to wait for. It does not need to be null terminated, but it must be shorter than PATH_MAX.
/// @param poll_interval The time to sleep between two consecutive `stat` calls.
/// @return An OsalReturnType indicating the result of the operation.
///         - `OsalReturnType::kSuccess`: The path exists.
///         - `OsalReturnType::kTimeout`: The path did not appear within the specified time.
///         - `OsalReturnType::kFail`: The path is empty, is too long, or could not be queried (e.g. permission
///           denied on one of the parent directories).
OsalReturnType wait_for_file(
    std::chrono::milliseconds timeout,
    std::string_view path,
    std::chrono::milliseconds poll_interval = kDefaultFilePollInterval) noexcept;

}  // namespace osal

}  // namespace internal

}  // namespace mw::lifecycle

}  // namespace score

#endif  // OSAL_WAIT_FOR_FILE_HPP_INCLUDED

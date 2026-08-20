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

#include <sys/stat.h>
#include <limits.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <string_view>
#include <thread>

#include "score/mw/launch_manager/osal/wait_for_file.hpp"

namespace score::mw::lifecycle::internal::osal
{

namespace
{

/// @brief Copy a possibly non null terminated path into a null terminated buffer.
/// @return true if the path fits into the buffer, false otherwise.
bool toNullTerminatedPath(std::string_view path, std::array<char, PATH_MAX>& buffer) noexcept
{
    // One byte is reserved for the terminator, which is already in place because the buffer is value initialised.
    if (path.empty() || (path.size() >= buffer.size()))
    {
        return false;
    }

    static_cast<void>(std::copy(path.cbegin(), path.cend(), buffer.begin()));
    return true;
}

}  // namespace

OsalReturnType wait_for_file(
    std::chrono::milliseconds timeout,
    std::string_view path,
    std::chrono::milliseconds poll_interval) noexcept
{
    std::array<char, PATH_MAX> path_buffer{};
    if (!toNullTerminatedPath(path, path_buffer))
    {
        return OsalReturnType::kFail;
    }

    // Calculate when the timeout will be reached. This avoids accumulating errors because `sleep_for` may block for
    // longer than requested.
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        struct stat info{};

        if (stat(path_buffer.data(), &info) == 0)
        {
            return OsalReturnType::kSuccess;
        }

        switch (errno)
        {
            // The path, or one of its parent directories, does not exist yet. This is what we are waiting for.
            case (ENOENT):
            case (ENOTDIR):
            case (EINTR):
                break;

            default:
                return OsalReturnType::kFail;
        }

        if (std::chrono::steady_clock::now() >= deadline)
        {
            return OsalReturnType::kTimeout;
        }

        std::this_thread::sleep_for(poll_interval);
    }
}

}  // namespace score::mw::lifecycle::internal::osal

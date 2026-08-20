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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <string_view>
#include <thread>

#include "score/mw/launch_manager/osal/wait_for_file.hpp"

namespace score::mw::lifecycle::internal::osal
{

OsalReturnType wait_for_file(
    std::string_view path,
    configuration::FileExistenceState condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds poll_interval) noexcept
{
    // note: QNX has a wait_for API, however this

    // The terminator is expected right behind the view, so it is not part of its size.
    if (path.empty() || (path.data()[path.size()] != '\0'))
    {
        return OsalReturnType::kFail;
    }

    const bool wait_for_existence = (condition == configuration::FileExistenceState::Exists);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        struct stat info{};

        if (stat(path.data(), &info) == 0)
        {
            if (wait_for_existence)
            {
                return OsalReturnType::kSuccess;
            }
        }
        else
        {
            switch (errno)
            {
                // The path, or one of its parent directories, does not exist.
                case (ENOENT):
                case (ENOTDIR):
                    if (!wait_for_existence)
                    {
                        return OsalReturnType::kSuccess;
                    }
                    break;

                // The query was interrupted, so it says nothing about the path. Simply retry it.
                case (EINTR):
                    break;

                default:
                    return OsalReturnType::kFail;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            return OsalReturnType::kTimeout;
        }

        // Never sleep past the deadline, so the timeout is honoured even with a coarse poll interval.
        const auto remaining = deadline - now;
        std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(poll_interval, remaining));
    }
}

}  // namespace score::mw::lifecycle::internal::osal

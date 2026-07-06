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
#include <charconv>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

constexpr std::string_view fd_dir_path{"/proc/self/fd"};

inline std::ostream& operator<<(std::ostream& outstream, const std::vector<std::pair<std::uint32_t, std::string>>& data){
    for(auto &[fd, path]: data){
        outstream << "(FD: "<< fd << ") " << path << std::endl;
    }
    return outstream;
}

    /// @brief Returns a list of all open FDs, ignoring stdin, stdout & stderr.
    /// @note This opens another FD for the /proc/self/fd directory however this is
    ///       not returned in the list.
    inline std::vector<std::pair<std::uint32_t, std::string>>
    get_fds()
{
    std::vector<std::pair<std::uint32_t, std::string>> out_vector{};

    DIR* fd_dir = ::opendir(fd_dir_path.begin());
    if (fd_dir == nullptr)
    {
        return out_vector;
    }
    int fd_dir_fd = dirfd(fd_dir);

    int fd_number{0};

    for (dirent* entry = ::readdir(fd_dir); entry != nullptr; entry = ::readdir(fd_dir))
    {
        auto result = std::from_chars(entry->d_name, std::next(entry->d_name, std::strlen(entry->d_name)), fd_number);
        if (result.ec != std::errc{})
        {
            continue;
        }

        // skip irelevant FDs
        if (fd_number == STDIN_FILENO || fd_number == STDOUT_FILENO || fd_number == STDERR_FILENO ||
            fd_number == fd_dir_fd)
        {
            continue;
        }

        auto& emplace_it = out_vector.emplace_back(static_cast<std::uint32_t>(fd_number), std::string(PATH_MAX, 'a'));

        const std::string fd_path = std::string{fd_dir_path} + "/" + entry->d_name;
        ssize_t len = ::readlink(fd_path.c_str(), emplace_it.second.data(), emplace_it.second.size());

        if (len <= 0)
        {
            emplace_it.second = "Could not get real path";
        }
        else
        {
            emplace_it.second.resize(len);
        }
    }

    ::closedir(fd_dir);
    return out_vector;
}

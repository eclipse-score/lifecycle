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
#ifndef PROCESS_COMMAND_HPP
#define PROCESS_COMMAND_HPP

#include <string>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

/// @brief Stores the executable path and arguments of a process.
///
/// Provides a null-terminated pointer array, starting with the executable path, suitable for use with the
/// @c execve and @c posix_spawn system calls via @ref argv().
class ProcessCommand
{
  public:
    ProcessCommand() = default;
    /// @brief Constructs the command from @p executable_path and the process arguments @p args.
    ProcessCommand(std::string executable_path, std::vector<std::string> args);

    ProcessCommand(const ProcessCommand&) = delete;
    ProcessCommand& operator=(const ProcessCommand&) = delete;
    ProcessCommand(ProcessCommand&& other) noexcept;
    ProcessCommand& operator=(ProcessCommand&& other) noexcept;

    ~ProcessCommand() = default;

    /// @brief Returns the path of the executable.
    [[nodiscard]] const std::string& executablePath() const;
    /// @brief Returns the process arguments, excluding the executable path.
    [[nodiscard]] const std::vector<std::string>& arguments() const;
    /// @brief Returns a null-terminated array of C strings, starting with the executable path, for use with
    /// @c execve or @c posix_spawn.
    [[nodiscard]] char* const* argv() const;

  private:
    void rebuildPointers();
    std::string executable_path_;
    std::vector<std::string> args_;
    // Default-initialized so argv() is valid even for a default constructed object.
    std::vector<const char*> pointers_{executable_path_.c_str(), nullptr};
};

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // PROCESS_COMMAND_HPP

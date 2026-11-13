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
#include "score/mw/launch_manager/configuration/process_command.hpp"

#include <utility>

namespace score::mw::lifecycle::internal::configuration
{

ProcessCommand::ProcessCommand(std::string executable_path, std::vector<std::string> args)
    : executable_path_{std::move(executable_path)}, args_{std::move(args)}
{
    rebuildPointers();
}

ProcessCommand::ProcessCommand(ProcessCommand&& other) noexcept
    : executable_path_{std::move(other.executable_path_)}, args_{std::move(other.args_)}
{
    rebuildPointers();
}

ProcessCommand& ProcessCommand::operator=(ProcessCommand&& other) noexcept
{
    if (this != &other)
    {
        executable_path_ = std::move(other.executable_path_);
        args_ = std::move(other.args_);
        rebuildPointers();
    }
    return *this;
}

const std::string& ProcessCommand::executablePath() const
{
    return executable_path_;
}

const std::vector<std::string>& ProcessCommand::arguments() const
{
    return args_;
}

char* const* ProcessCommand::argv() const
{
    // const_cast is required to convert to the type expected by execve / posix_spawn
    return const_cast<char* const*>(pointers_.data());
}

void ProcessCommand::rebuildPointers()
{
    pointers_.clear();
    // +2 for the executable path and the nullptr terminator
    pointers_.reserve(args_.size() + 2);
    pointers_.push_back(executable_path_.c_str());
    for (const auto& arg : args_)
    {
        pointers_.push_back(arg.c_str());
    }
    pointers_.push_back(nullptr);
}

}  // namespace score::mw::lifecycle::internal::configuration

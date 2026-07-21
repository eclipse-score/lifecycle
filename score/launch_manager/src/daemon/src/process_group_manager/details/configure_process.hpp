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

#include <string_view>

#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

/// @brief Logs a message in an async signal safe way, then ends the process.
void safe_log_and_exit(std::initializer_list<std::string_view> parts);

/// @brief Logs a message and errno in an async signal safe way, then ends the process.
void safe_log_errno_and_exit(std::initializer_list<std::string_view> parts);

/// @brief Sets the limit if given a non-zero value, otherwise skips.
/// @warning This will exit with failure if not successful.
void setLimit(const int resource, const std::size_t amount, const std::string_view rlimit_name);

/// @warning This will exit with failure if not successful.
void setSchedulingAndSecurity(const osal::OsalConfig& config);

/// @warning This will exit with failure if not successful.
void changeCurrentWorkingDirectory(const osal::OsalConfig& config);

/// @warning This will exit with failure if not successful.
void implementMemoryResourceLimits(const osal::OsalConfig& config);

/// @warning This will exit with failure if not successful.
void changeSecurityPolicy(const osal::OsalConfig& config);

}  // namespace internal

}  // namespace lcm

}  // namespace score

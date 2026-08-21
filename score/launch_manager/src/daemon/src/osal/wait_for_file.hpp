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

#include <score/stop_token.hpp>

#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/os/stat.h"
#include <chrono>
#include <cstdint>
#include <string_view>

#include "return_types.hpp"

namespace score::mw::lifecycle::internal::osal
{

/// @brief Block until the given path reaches the requested state, the timeout elapses, or a stop is requested.
///
/// @param path The path to wait for. It must be null terminated.
/// @param condition The path state to wait for.
/// @param timeout The maximum time to wait for the condition.
/// @param poll_interval The time between two consecutive existence checks.
/// @param stat_os Optional score::os::Stat instance used to query the path.
OsalReturnType wait_for_file(
    std::string_view path,
    configuration::FileExistenceState condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds poll_interval,
    const score::os::Stat& stat_os = score::os::Stat::instance()) noexcept;

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_WAIT_FOR_FILE_HPP_INCLUDED

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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_LOGGER_H
#define SCORE_HEALTH_MONITOR_INTERNAL_LOGGER_H

#include "score/mw/log/log_stream.h"
#include "score/mw/log/logger.h"

#include <string_view>

namespace score::health_monitor::details
{

/// @brief Log context of every health monitor message.
const std::string_view kLogContext{"HMON"};

/// @brief Returns the shared logger with context `kLogContext`.
inline const score::mw::log::Logger& GetLogger() noexcept
{
    static const score::mw::log::Logger instance{kLogContext};
    return instance;
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_LOGGER_H

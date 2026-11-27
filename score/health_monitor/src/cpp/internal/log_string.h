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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_LOG_STRING_H
#define SCORE_HEALTH_MONITOR_INTERNAL_LOG_STRING_H

#include "score/health_monitor/tag.h"
#include "score/mw/log/log_types.h"

namespace score::health_monitor::details
{

/// @brief Returns the text of `tag` in the form the log stream takes.
template <typename Kind>
constexpr score::mw::log::LogString ToLogString(const Tag<Kind>& tag) noexcept
{
    return score::mw::log::LogString{tag.Data(), tag.Size()};
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_LOG_STRING_H

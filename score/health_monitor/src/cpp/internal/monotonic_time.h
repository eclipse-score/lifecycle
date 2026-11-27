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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_MONOTONIC_TIME_H
#define SCORE_HEALTH_MONITOR_INTERNAL_MONOTONIC_TIME_H

#include "score/os/time.h"

#include <score/assert.hpp>

#include <chrono>
#include <ctime>

namespace score::health_monitor::details
{

/// @brief Reads `CLOCK_MONOTONIC` through `time` as a `std::chrono::steady_clock` time point.
/// @note `steady_clock` reads `CLOCK_MONOTONIC` on Linux and QNX, so the result compares with `steady_clock::now()`.
/// The process aborts if the read fails.
inline std::chrono::steady_clock::time_point ReadMonotonicTime(const score::os::Time& time) noexcept
{
    timespec now{};
    const auto result = time.clock_gettime(CLOCK_MONOTONIC, &now);
    // The read fails only for an invalid clock or pointer, so a failure is a defect and not a runtime condition.
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), "CLOCK_MONOTONIC read failed");
    return std::chrono::steady_clock::time_point{std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::seconds{now.tv_sec} + std::chrono::nanoseconds{now.tv_nsec})};
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_MONOTONIC_TIME_H

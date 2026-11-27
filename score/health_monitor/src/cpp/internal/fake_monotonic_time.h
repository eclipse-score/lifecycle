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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_FAKE_MONOTONIC_TIME_H
#define SCORE_HEALTH_MONITOR_INTERNAL_FAKE_MONOTONIC_TIME_H

#include "score/os/mocklib/time_mock.h"

#include <gmock/gmock.h>

#include <chrono>
#include <cstdint>
#include <ctime>

namespace score::health_monitor::details
{

/// @brief Converts `time` to the `timespec` that `clock_gettime` fills.
inline timespec ToTimespec(const std::chrono::steady_clock::time_point time) noexcept
{
    const auto since_epoch = time.time_since_epoch();
    const auto seconds = std::chrono::floor<std::chrono::seconds>(since_epoch);
    return timespec{
        static_cast<std::time_t>(seconds.count()),
        static_cast<long>(std::chrono::nanoseconds{since_epoch - seconds}.count())};
}

/// @brief Makes `mock` report the value of `now` at each `CLOCK_MONOTONIC` read.
/// @pre `now` outlives every read through `mock`.
/// @warning Do not change `now` while another thread reads the clock.
/// @note A read of any other clock fails the test.
inline void ReportMonotonicTime(score::os::TimeMock& mock, const std::chrono::steady_clock::time_point& now)
{
    EXPECT_CALL(mock, clock_gettime(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mock, clock_gettime(CLOCK_MONOTONIC, ::testing::_))
        .WillRepeatedly([&now](const clockid_t, timespec* const tp) {
            *tp = ToTimespec(now);
            return score::cpp::expected<std::int32_t, score::os::Error>{0};
        });
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_FAKE_MONOTONIC_TIME_H

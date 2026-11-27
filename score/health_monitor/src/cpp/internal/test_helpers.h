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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_TEST_HELPERS_H
#define SCORE_HEALTH_MONITOR_INTERNAL_TEST_HELPERS_H

#include "score/health_monitor/error.h"
#include "score/result/result.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace score::health_monitor::details
{

constexpr std::chrono::seconds kWaitTimeout{10};

/// @brief Polls `condition` every millisecond until it holds or the timeout passes, and returns whether it held.
template <typename Condition>
bool WaitFor(Condition condition, const std::chrono::steady_clock::duration timeout = kWaitTimeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!condition())
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return true;
}

/// @brief Fails the calling test unless `result` holds the error `code`.
template <typename T>
void ExpectError(const score::Result<T>& result, const ErrorCode code)
{
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(code));
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_TEST_HELPERS_H

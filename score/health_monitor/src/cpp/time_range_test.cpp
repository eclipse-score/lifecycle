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

#include "score/health_monitor/time_range.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>

using namespace score::health_monitor;
using std::chrono::milliseconds;

TEST(TimeRangeTest, ReturnsMinAndMaxInMilliseconds)
{
    const TimeRange range{milliseconds{10}, milliseconds{250}};

    EXPECT_EQ(range.Min(), milliseconds{10});
    EXPECT_EQ(range.Max(), milliseconds{250});
}

TEST(TimeRangeTest, MinEqualToMaxIsAllowed)
{
    const TimeRange range{milliseconds{42}, milliseconds{42}};

    EXPECT_EQ(range.Min(), milliseconds{42});
    EXPECT_EQ(range.Max(), milliseconds{42});
}

TEST(TimeRangeTest, ZeroRangeIsAllowed)
{
    const TimeRange range{milliseconds{0}, milliseconds{0}};

    EXPECT_EQ(range.Min(), milliseconds{0});
    EXPECT_EQ(range.Max(), milliseconds{0});
}

TEST(TimeRangeTest, ValuesAbove32BitAreKept)
{
    const std::int64_t kBeyond32Bit{(std::int64_t{1} << 32) + 5};
    const TimeRange range{milliseconds{kBeyond32Bit}, milliseconds{kBeyond32Bit + 1}};

    EXPECT_EQ(range.Min(), milliseconds{kBeyond32Bit});
    EXPECT_EQ(range.Max(), milliseconds{kBeyond32Bit + 1});
}

TEST(TimeRangeDeathTest, MinAboveMaxAborts)
{
    EXPECT_DEATH((TimeRange{milliseconds{11}, milliseconds{10}}), "");
}

TEST(TimeRangeDeathTest, NegativeMinAborts)
{
    EXPECT_DEATH((TimeRange{milliseconds{-1}, milliseconds{10}}), "");
}

TEST(TimeRangeDeathTest, NegativeMaxAborts)
{
    EXPECT_DEATH((TimeRange{milliseconds{-2}, milliseconds{-1}}), "");
}

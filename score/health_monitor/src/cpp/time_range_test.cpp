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
#include <ratio>
#include <type_traits>

using namespace score::health_monitor;
using std::chrono::milliseconds;

TEST(TimeRangeTest, ReturnsMinAndMaxInMilliseconds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_granularity");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a range keeps its minimum and maximum as whole milliseconds.");

    const TimeRange range{milliseconds{10}, milliseconds{250}};

    EXPECT_EQ(range.Min(), milliseconds{10});
    EXPECT_EQ(range.Max(), milliseconds{250});
}

TEST(TimeRangeTest, MinEqualToMaxIsAllowed)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_time_range");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a range whose minimum equals its maximum is accepted.");

    const TimeRange range{milliseconds{42}, milliseconds{42}};

    EXPECT_EQ(range.Min(), milliseconds{42});
    EXPECT_EQ(range.Max(), milliseconds{42});
}

TEST(TimeRangeTest, ZeroRangeIsAllowed)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_time_range");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a range of zero to zero milliseconds is accepted.");

    const TimeRange range{milliseconds{0}, milliseconds{0}};

    EXPECT_EQ(range.Min(), milliseconds{0});
    EXPECT_EQ(range.Max(), milliseconds{0});
}

TEST(TimeRangeTest, ValuesAbove32BitAreKept)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_granularity");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that millisecond values above 32 bits are kept without truncation.");

    const std::int64_t kBeyond32Bit{(std::int64_t{1} << 32) + 5};
    const TimeRange range{milliseconds{kBeyond32Bit}, milliseconds{kBeyond32Bit + 1}};

    EXPECT_EQ(range.Min(), milliseconds{kBeyond32Bit});
    EXPECT_EQ(range.Max(), milliseconds{kBeyond32Bit + 1});
}

TEST(TimeRangeTest, SubMillisecondValuesDoNotCompile)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_sub_ms");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that a range cannot be built from microseconds or fractional milliseconds, so a value that is not a "
        "whole number of milliseconds is rejected at compile time instead of being truncated.");

    static_assert(
        std::is_constructible<TimeRange, milliseconds, milliseconds>::value, "Whole milliseconds must be accepted.");
    static_assert(
        std::is_constructible<TimeRange, std::chrono::seconds, std::chrono::seconds>::value,
        "Coarser units convert to whole milliseconds and must be accepted.");
    static_assert(
        !std::is_constructible<TimeRange, std::chrono::microseconds, std::chrono::microseconds>::value,
        "Microseconds must not convert to milliseconds.");
    static_assert(
        !std::is_constructible<
            TimeRange,
            std::chrono::duration<double, std::milli>,
            std::chrono::duration<double, std::milli>>::value,
        "Fractional milliseconds must not convert to milliseconds.");
}

TEST(TimeRangeDeathTest, MinAboveMaxAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_time_range");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a minimum one millisecond above the maximum aborts.");

    EXPECT_DEATH((TimeRange{milliseconds{11}, milliseconds{10}}), "");
}

TEST(TimeRangeDeathTest, NegativeMinAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_time_range");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a negative minimum aborts.");

    EXPECT_DEATH((TimeRange{milliseconds{-1}, milliseconds{10}}), "");
}

TEST(TimeRangeDeathTest, NegativeMaxAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_time_range");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a negative maximum aborts.");

    EXPECT_DEATH((TimeRange{milliseconds{-2}, milliseconds{-1}}), "");
}

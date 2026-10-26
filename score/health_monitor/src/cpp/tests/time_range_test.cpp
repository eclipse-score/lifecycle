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

#include "score/mw/health/common.h"
#include <gtest/gtest.h>
#include <cstdint>

using namespace score::mw::health;

class TimeRangeFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(TimeRangeFixture, New_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed.");
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};
}

TEST_F(TimeRangeFixture, New_InvalidOrder)
{
    RecordProperty("Description", "Object failed to construct to invalid parameters order.");
    using namespace std::chrono_literals;
    // `SIGABRT` is expected.
    ASSERT_DEATH({ TimeRange range(200ms, 100ms); }, "");
}

TEST_F(TimeRangeFixture, MinMax)
{
    RecordProperty("Description", "`min` and `max` member functions are returning correct values.");
    using namespace std::chrono_literals;
    TimeRange range{123ms, 456ms};
    ASSERT_EQ(range.MinMs(), 123);
    ASSERT_EQ(range.MaxMs(), 456);
}

TEST_F(TimeRangeFixture, GivenMinOutOfBounds_WhenConstructorCalled_ThenCrash)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a negative time (-1). "
        "When constructor is called. "
        "Then the out of bounds values cause a crash.");

    // Given
    constexpr std::chrono::milliseconds::rep OUT_OF_BOUNDS_MIN_COUNT = static_cast<std::chrono::milliseconds::rep>(-1);
    constexpr std::chrono::milliseconds::rep VALID_MAX_COUNT = 1;

    // When, Then
    ASSERT_DEATH(
        {
            TimeRange range(
                std::chrono::milliseconds{OUT_OF_BOUNDS_MIN_COUNT}, std::chrono::milliseconds{VALID_MAX_COUNT});
        },
        "");
}

TEST_F(TimeRangeFixture, GivenMaxInt64_WhenConstructorCalled_ThenSuccess)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given the largest representable maximum (INT64_MAX). "
        "When constructor is called. "
        "Then the maximum bound is returned successfully from MaxMs().");

    // Given
    constexpr std::chrono::milliseconds::rep VALID_MIN_COUNT = 0;
    constexpr std::chrono::milliseconds::rep MAX_COUNT = INT64_MAX;

    // When
    TimeRange range(std::chrono::milliseconds{VALID_MIN_COUNT}, std::chrono::milliseconds{MAX_COUNT});

    // Then
    EXPECT_EQ(range.MaxMs(), static_cast<std::uint64_t>(INT64_MAX));
}

TEST_F(TimeRangeFixture, GivenMaxUint64_WhenConstructorCalled_ThenCrash)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given the maximum value of the storage type (UINT64_MAX). "
        "When constructor is called. "
        "Then the chrono::milliseconds type wraps on construction, causing crash.");

    // Given
    constexpr std::chrono::milliseconds::rep VALID_MIN_COUNT = 0;
    constexpr std::chrono::milliseconds::rep MAX_COUNT = UINT64_MAX;

    // When, Then
    ASSERT_DEATH(
        { TimeRange range(std::chrono::milliseconds{VALID_MIN_COUNT}, std::chrono::milliseconds{MAX_COUNT}); }, "");
}

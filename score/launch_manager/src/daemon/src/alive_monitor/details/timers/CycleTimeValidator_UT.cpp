/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimeValidator.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"

using namespace testing;
using namespace std::chrono_literals;

namespace score::mw::lifecycle::internal::saf::timers
{

inline auto ReturnClockResolution(const struct timespec& resolution)
{
    return Invoke([resolution](struct timespec* ts) {
        *ts = resolution;
        return 0;
    });
}

class MockOsClockInterface : public OsClockInterface
{
  public:
    MOCK_METHOD(int, clockGetRes, (struct timespec*), (const, noexcept, override));
};

class CycleTimeValidatorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
    }

    MockOsClockInterface mock_clock_;
};

TEST_F(CycleTimeValidatorTest, GetMonotonicClockAccuracySuccessful)
{
    RecordProperty("Description", "Verify getMonotonicClockAccuracy returns accuracy successfully");

    struct timespec resolution{0, 1000000};  // 1 millisecond

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds result = CycleTimeValidator::getMonotonicClockAccuracy(mock_clock_);

    EXPECT_EQ(result.count(), 1000000);
}

TEST_F(CycleTimeValidatorTest, GetMonotonicClockAccuracyZeroNanoseconds)
{
    RecordProperty("Description", "Verify getMonotonicClockAccuracy with zero nanoseconds resolution");

    struct timespec resolution{0, 0};

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds result = CycleTimeValidator::getMonotonicClockAccuracy(mock_clock_);

    EXPECT_EQ(result.count(), 0);
}

TEST_F(CycleTimeValidatorTest, GetMonotonicClockAccuracyLargeValue)
{
    RecordProperty("Description", "Verify getMonotonicClockAccuracy with large nanoseconds value");

    struct timespec resolution{0, 999999999};  // Almost 1 second

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds result = CycleTimeValidator::getMonotonicClockAccuracy(mock_clock_);

    EXPECT_EQ(result.count(), 999999999);
}

TEST_F(CycleTimeValidatorTest, GetMonotonicClockAccuracyFails)
{
    RecordProperty("Description", "Verify getMonotonicClockAccuracy returns -1 on clock failure");

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(Return(-1));

    std::chrono::nanoseconds result = CycleTimeValidator::getMonotonicClockAccuracy(mock_clock_);

    EXPECT_EQ(result.count(), -1);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyReturnsRequestedTime)
{
    RecordProperty(
        "Description",
        "Verify adjustCycleTimeOnClockAccuracy returns requested time when "
        "requested is >= accuracy");

    struct timespec resolution{0, 1000000};  // 1 millisecond

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{10000000ns};  // 10 milliseconds
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), requested.count());
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyReturnsAccuracy)
{
    RecordProperty(
        "Description",
        "Verify adjustCycleTimeOnClockAccuracy returns accuracy when "
        "requested is < accuracy");

    struct timespec resolution{0, 10000000};  // 10 milliseconds

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{1000000ns};  // 1 millisecond
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), 10000000);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyZeroRequested)
{
    RecordProperty("Description", "Verify adjustCycleTimeOnClockAccuracy with zero requested time");

    struct timespec resolution{0, 1000000};  // 1 millisecond

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{0ns};
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), 1000000);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyNegativeRequested)
{
    RecordProperty("Description", "Verify adjustCycleTimeOnClockAccuracy with negative requested time");

    struct timespec resolution{0, 1000000};  // 1 millisecond

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{-1ns};
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), 1000000);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyClockResolutionFails)
{
    RecordProperty("Description", "Verify adjustCycleTimeOnClockAccuracy returns -1 when clock resolution fails");

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(Return(-1));

    std::chrono::nanoseconds requested{10000000ns};
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), -1);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyEqualValues)
{
    RecordProperty("Description", "Verify adjustCycleTimeOnClockAccuracy when requested equals accuracy");

    struct timespec resolution{0, 5000000};  // 5 milliseconds

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{5000000ns};
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result.count(), 5000000);
}

TEST_F(CycleTimeValidatorTest, AdjustCycleTimeOnClockAccuracyLargeValues)
{
    RecordProperty("Description", "Verify adjustCycleTimeOnClockAccuracy with large values");

    struct timespec resolution{0, 100000};  // 100 microseconds

    EXPECT_CALL(mock_clock_, clockGetRes(_)).WillOnce(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{60s};
    std::chrono::nanoseconds result = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result, 60s);
}

TEST_F(CycleTimeValidatorTest, MultipleCallsConsistent)
{
    RecordProperty("Description", "Verify multiple calls return consistent results");

    struct timespec resolution{0, 1000000};  // 1 millisecond

    EXPECT_CALL(mock_clock_, clockGetRes(_)).Times(2).WillRepeatedly(ReturnClockResolution(resolution));

    std::chrono::nanoseconds requested{10000000ns};
    std::chrono::nanoseconds result1 = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);
    std::chrono::nanoseconds result2 = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(requested, mock_clock_);

    EXPECT_EQ(result1, result2);
}

}  // namespace score::mw::lifecycle::internal::saf::timers

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

#include <ctime>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"

using namespace testing;
using namespace std::chrono_literals;

namespace score::mw::lifecycle::internal::saf::timers
{

class MockOsClockInterface : public OsClockInterface
{
  public:
    MOCK_METHOD(int, clockGetTime, (struct timespec * ts), (const, noexcept, override));
    MOCK_METHOD(
        int,
        clockNanosleep,
        (int flags, const struct timespec* request, struct timespec* remain),
        (const, noexcept, override));
};

class CycleTimerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }

    // Helper to set time on mock
    void SetMockTime(MockOsClockInterface& mock, struct timespec ts)
    {
        ON_CALL(mock, clockGetTime).WillByDefault(DoAll(SetArgPointee<0>(ts), Return(0)));
    }
};

// --------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------

TEST_F(CycleTimerTest, Constructor_WithValidInterface_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CycleTimer can be constructed with a valid OS clock interface.");

    MockOsClockInterface mockClock;
    CycleTimer timer(&mockClock);

    // No crash should occur
}

TEST_F(CycleTimerTest, Constructor_WithNullInterface_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CycleTimer can be constructed with a null interface.");

    CycleTimer timer(nullptr);

    // No crash should occur
}

// --------------------------------------------------------------------------
// init
// --------------------------------------------------------------------------

TEST_F(CycleTimerTest, Init_WithNullInterface_ReturnsErrorMinusTwo)
{
    RecordProperty("Description", "Verify that init returns -2 when interface is null.");

    CycleTimer timer(nullptr);
    auto result = timer.init(10000ns);

    ASSERT_EQ(result, std::chrono::nanoseconds{-2});
}

TEST_F(CycleTimerTest, Init_WithInvalidInterval_ReturnsErrorMinusThree)
{
    RecordProperty("Description", "Verify that init returns -3 when sleep interval is invalid (zero).");

    MockOsClockInterface mockClock;
    CycleTimer timer(&mockClock);

    auto result = timer.init(0ns);
    ASSERT_EQ(result, std::chrono::nanoseconds{-3});
}

TEST_F(CycleTimerTest, Init_WithNegativeInterval_ReturnsErrorMinusThree)
{
    RecordProperty("Description", "Verify that init returns -3 when sleep interval is negative.");

    MockOsClockInterface mockClock;
    CycleTimer timer(&mockClock);

    auto result = timer.init(std::chrono::nanoseconds{-1});
    ASSERT_EQ(result, std::chrono::nanoseconds{-3});
}

TEST_F(CycleTimerTest, Init_WithClockGetTimeFailure_ReturnsErrorMinusOne)
{
    RecordProperty("Description", "Verify that init returns -1 when clockGetTime fails.");

    MockOsClockInterface mockClock;
    EXPECT_CALL(mockClock, clockGetTime).WillOnce(Return(-1));

    CycleTimer timer(&mockClock);
    auto result = timer.init(10000ns);

    ASSERT_EQ(result, std::chrono::nanoseconds{-1});
}

TEST_F(CycleTimerTest, Init_WithValidInterval_ReturnsInterval)
{
    RecordProperty("Description", "Verify that init returns the configured interval on success.");

    MockOsClockInterface mockClock;
    struct timespec ts{1, 500000000};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    std::chrono::nanoseconds interval = 10000ns;
    auto result = timer.init(interval);

    ASSERT_EQ(result, interval);
}

// --------------------------------------------------------------------------
// start
// --------------------------------------------------------------------------

TEST_F(CycleTimerTest, Start_ReturnsCurrentTimestamp)
{
    RecordProperty("Description", "Verify that start returns the current timestamp on success.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 123456789};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(10000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    auto result = timer.start();

    // 10 seconds + 123456789 nanoseconds = 10123456789 nanoseconds
    ASSERT_EQ(result.count(), 10123456789);
}

TEST_F(CycleTimerTest, Start_WithClockGetTimeFailure_ReturnsZero)
{
    RecordProperty("Description", "Verify that start returns zero when clockGetTime fails.");

    MockOsClockInterface mockClock;
    struct timespec ts{0, 0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(10000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(Return(-1));
    auto result = timer.start();

    ASSERT_EQ(result, 0ns);
}

// --------------------------------------------------------------------------
// calcNextShot
// --------------------------------------------------------------------------

TEST_F(CycleTimerTest, CalcNextShot_AddsIntervalToDeadline)
{
    RecordProperty("Description", "Verify that calcNextShot correctly adds the sleep interval to the deadline.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 500000000};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    std::chrono::nanoseconds interval = 1000000000ns;  // 1 second
    timer.init(interval);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    struct timespec& nextShot = timer.calcNextShot();

    // Initial deadline: 10.5s, add 1.0s = 11.5s
    ASSERT_EQ(nextShot.tv_sec, 11);
    ASSERT_EQ(nextShot.tv_nsec, 500000000);
}

TEST_F(CycleTimerTest, CalcNextShot_HandlesNanosecondOverflow)
{
    RecordProperty("Description", "Verify that calcNextShot correctly handles nanosecond overflow to seconds.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 900000000};  // 10.9s

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    std::chrono::nanoseconds interval = 200000000ns;  // 0.2 seconds
    timer.init(interval);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    struct timespec& nextShot = timer.calcNextShot();

    // 10.9s + 0.2s = 11.1s
    ASSERT_EQ(nextShot.tv_sec, 11);
    ASSERT_EQ(nextShot.tv_nsec, 100000000);
}

TEST_F(CycleTimerTest, CalcNextShot_MultipleOverflows)
{
    RecordProperty("Description", "Verify that calcNextShot correctly handles multiple nanosecond overflows.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 999999999};  // 10.999999999s

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    std::chrono::nanoseconds interval = 2000000000ns;  // 2 seconds
    timer.init(interval);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    struct timespec& nextShot = timer.calcNextShot();

    // 10.999999999s + 2s = 12.999999999s
    ASSERT_EQ(nextShot.tv_sec, 12);
    ASSERT_EQ(nextShot.tv_nsec, 999999999);
}

// --------------------------------------------------------------------------
// sleep
// --------------------------------------------------------------------------

TEST_F(CycleTimerTest, Sleep_WithDeadlineNotReached_SucceedsSleep)
{
    RecordProperty("Description", "Verify that sleep successfully waits when deadline has not been reached.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(100000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    timer.calcNextShot();

    // Set up next call to be before deadline
    struct timespec ts_before{10, 50000};  // Before the deadline
    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts_before), Return(0)));
    EXPECT_CALL(mockClock, clockNanosleep(TIMER_ABSTIME, _, _)).WillOnce(Return(0));

    std::atomic_bool terminate{false};
    std::chrono::nanoseconds overDeadline{0};
    int result = timer.sleep(terminate, overDeadline);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(overDeadline.count(), 0);
}

TEST_F(CycleTimerTest, Sleep_WithDeadlineAlreadyPassed_ReturnsDeadlineAlreadyOver)
{
    RecordProperty("Description", "Verify that sleep returns kDeadlineAlreadyOver when deadline has passed.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(100000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    timer.calcNextShot();

    // Set up next call to be after deadline
    struct timespec ts_after{10, 200000};  // After the deadline (which was 10, 100000)
    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts_after), Return(0)));

    std::atomic_bool terminate{false};
    std::chrono::nanoseconds overDeadline{0};
    int result = timer.sleep(terminate, overDeadline);

    ASSERT_EQ(result, CycleTimer::kDeadlineAlreadyOver);
    ASSERT_GT(overDeadline.count(), 0);
}

TEST_F(CycleTimerTest, Sleep_WithTerminationSignal_BreaksFromLoop)
{
    RecordProperty("Description", "Verify that sleep respects termination signal and breaks from loop.");

    MockOsClockInterface mockClock;
    struct timespec ts{10, 0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(1000000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    timer.start();

    timer.calcNextShot();

    // Set termination flag from the start
    std::atomic_bool terminate{true};
    std::chrono::nanoseconds overDeadline{0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    const int result = timer.sleep(terminate, overDeadline);

    ASSERT_EQ(result, EINTR);
}

}  // namespace score::mw::lifecycle::internal::saf::timers

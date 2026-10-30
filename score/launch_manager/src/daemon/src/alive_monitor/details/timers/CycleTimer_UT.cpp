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
    }
};

class CycleTimerConstructorTest : public CycleTimerTest
{
};

TEST_F(CycleTimerConstructorTest, Constructor_WithNullInterface_InitializesSuccessfully)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that CycleTimer can be constructed with a null interface.");

    CycleTimer timer(nullptr);

    // No crash should occur
}

class CycleTimerInitTest : public CycleTimerTest
{
};

TEST_F(CycleTimerInitTest, Init_NullInterfaceAndValidInterval_ReturnsErrorMinusTwo)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that init returns -2 when interface is null and the interval is valid.");

    CycleTimer timer(nullptr);
    auto result = timer.init(10000ns);

    EXPECT_EQ(result, CycleTimer::kInitNullInterface);
}

TEST_F(CycleTimerInitTest, Init_ValidInterfaceAndZeroInterval_ReturnsErrorMinusThree)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Verify that init returns -3 when interface is valid and the interval is zero.");

    MockOsClockInterface mockClock;
    CycleTimer timer(&mockClock);

    auto result = timer.init(0ns);
    EXPECT_EQ(result, CycleTimer::kInitInvalidInterval);
}

TEST_F(CycleTimerInitTest, Init_ValidInterfaceAndNegativeInterval_ReturnsErrorMinusThree)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Verify that init returns -3 when interface is valid and the interval is negative.");

    MockOsClockInterface mockClock;
    CycleTimer timer(&mockClock);

    auto result = timer.init(std::chrono::nanoseconds{-1});
    EXPECT_EQ(result, CycleTimer::kInitInvalidInterval);
}

TEST_F(CycleTimerInitTest, Init_NullInterfaceAndInvalidInterval_ReturnsErrorMinusTwo)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Verify that init returns -2 when interface is null and the interval is invalid; the interface check takes "
        "precedence.");
    RecordProperty(
        "Description",
        "Verify that init returns -2 when interface is null and the interval is invalid; the interface check takes "
        "precedence.");

    CycleTimer timer(nullptr);

    auto result = timer.init(0ns);
    EXPECT_EQ(result, CycleTimer::kInitNullInterface);
}

TEST_F(CycleTimerInitTest, Init_WithClockGetTimeFailure_ReturnsErrorMinusOne)
{
    RecordProperty("DerivationTechnique", "error-condition-injection");
    RecordProperty("Description", "Verify that init returns -1 when clockGetTime fails.");

    MockOsClockInterface mockClock;
    EXPECT_CALL(mockClock, clockGetTime).WillOnce(Return(-1));

    CycleTimer timer(&mockClock);
    auto result = timer.init(10000ns);

    EXPECT_EQ(result, std::chrono::nanoseconds{-1});
    EXPECT_EQ(result, std::chrono::nanoseconds{-1});
}

TEST_F(CycleTimerInitTest, Init_WithValidInterval_ReturnsInterval)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that init returns the configured interval on success.");

    MockOsClockInterface mockClock;
    struct timespec ts{1, 500000000};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    std::chrono::nanoseconds interval = 10000ns;
    auto result = timer.init(interval);

    EXPECT_EQ(result, interval);
    EXPECT_EQ(result, interval);
}

class CycleTimerStartTest : public CycleTimerTest
{
};

TEST_F(CycleTimerStartTest, Start_ReturnsCurrentTimestamp)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that start returns the current timestamp on success.");

    InSequence seq;
    MockOsClockInterface mockClock;
    struct timespec ts{10, 123456789};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(10000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));
    auto result = timer.start();

    // 10 seconds + 123456789 nanoseconds = 10123456789 nanoseconds
    EXPECT_EQ(result.count(), 10123456789);
    EXPECT_EQ(result.count(), 10123456789);
}

TEST_F(CycleTimerStartTest, Start_WithClockGetTimeFailure_ReturnsZero)
{
    RecordProperty("DerivationTechnique", "error-condition-injection");
    RecordProperty("Description", "Verify that start returns zero when clockGetTime fails.");

    InSequence seq;
    MockOsClockInterface mockClock;
    struct timespec ts{0, 0};

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(DoAll(SetArgPointee<0>(ts), Return(0)));

    CycleTimer timer(&mockClock);
    timer.init(10000ns);

    EXPECT_CALL(mockClock, clockGetTime).WillOnce(Return(-1));
    auto result = timer.start();

    EXPECT_EQ(result, 0ns);
    EXPECT_EQ(result, 0ns);
}

class CycleTimerCalcNextShotTest : public CycleTimerTest
{
};

TEST_F(CycleTimerCalcNextShotTest, CalcNextShot_AddsIntervalToDeadline)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that calcNextShot correctly adds the sleep interval to the deadline.");

    InSequence seq;
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
    EXPECT_EQ(nextShot.tv_sec, 11);
    EXPECT_EQ(nextShot.tv_nsec, 500000000);
    EXPECT_EQ(nextShot.tv_sec, 11);
    EXPECT_EQ(nextShot.tv_nsec, 500000000);
}

TEST_F(CycleTimerCalcNextShotTest, CalcNextShot_HandlesNanosecondOverflow)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Verify that calcNextShot correctly handles nanosecond overflow to seconds.");

    InSequence seq;
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
    EXPECT_EQ(nextShot.tv_sec, 11);
    EXPECT_EQ(nextShot.tv_nsec, 100000000);
    EXPECT_EQ(nextShot.tv_sec, 11);
    EXPECT_EQ(nextShot.tv_nsec, 100000000);
}

TEST_F(CycleTimerCalcNextShotTest, CalcNextShot_MultipleOverflows)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Verify that calcNextShot correctly handles multiple nanosecond overflows.");

    InSequence seq;
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
    EXPECT_EQ(nextShot.tv_sec, 12);
    EXPECT_EQ(nextShot.tv_nsec, 999999999);
    EXPECT_EQ(nextShot.tv_sec, 12);
    EXPECT_EQ(nextShot.tv_nsec, 999999999);
}

class CycleTimerSleepTest : public CycleTimerTest
{
};

TEST_F(CycleTimerSleepTest, Sleep_WithDeadlineNotReached_SucceedsSleep)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that sleep successfully waits when deadline has not been reached.");

    InSequence seq;
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

    EXPECT_EQ(result, 0);
    EXPECT_EQ(overDeadline.count(), 0);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(overDeadline.count(), 0);
}

TEST_F(CycleTimerSleepTest, Sleep_WithDeadlineAlreadyPassed_ReturnsDeadlineAlreadyOver)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that sleep returns kDeadlineAlreadyOver when deadline has passed.");

    InSequence seq;
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

    EXPECT_EQ(result, CycleTimer::kDeadlineAlreadyOver);
    EXPECT_EQ(overDeadline, 100000ns);
    EXPECT_EQ(result, CycleTimer::kDeadlineAlreadyOver);
    EXPECT_EQ(overDeadline, 100000ns);
}

TEST_F(CycleTimerSleepTest, Sleep_WithTerminationSignal_BreaksFromLoop)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Verify that sleep respects termination signal and breaks from loop.");

    InSequence seq;
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

    EXPECT_EQ(result, EINTR);
    EXPECT_EQ(result, EINTR);
}

}  // namespace score::mw::lifecycle::internal::saf::timers

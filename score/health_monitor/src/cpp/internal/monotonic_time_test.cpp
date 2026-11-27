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

#include "score/health_monitor/src/cpp/internal/monotonic_time.h"
#include "score/health_monitor/src/cpp/internal/fake_monotonic_time.h"

#include "score/os/time.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <tuple>

using namespace score::health_monitor::details;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

TEST(ReadMonotonicTimeTest, ConvertsSecondsAndNanoseconds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monotonic_time");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that the seconds and nanoseconds of CLOCK_MONOTONIC add up to one time point.");

    const std::chrono::steady_clock::time_point now{5s + 250ms + 7ns};
    StrictMock<score::os::TimeMock> time;
    ReportMonotonicTime(time, now);

    EXPECT_EQ(ReadMonotonicTime(time), now);
}

TEST(ReadMonotonicTimeTest, MatchesSteadyClock)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monotonic_time");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that the OS CLOCK_MONOTONIC read lies between two steady_clock reads.");

    const auto before = std::chrono::steady_clock::now();
    const auto read = ReadMonotonicTime(score::os::Time::instance());
    const auto after = std::chrono::steady_clock::now();

    EXPECT_LE(before, read);
    EXPECT_LE(read, after);
}

TEST(ReadMonotonicTimeTest, FailedReadAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a failed CLOCK_MONOTONIC read aborts.");

    StrictMock<score::os::TimeMock> time;
    EXPECT_CALL(time, clock_gettime(CLOCK_MONOTONIC, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    EXPECT_EXIT(std::ignore = ReadMonotonicTime(time), ::testing::KilledBySignal(SIGABRT), "");
}

TEST(ElapsedMsTest, CountsMillisecondsBetweenTimePoints)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monotonic_time");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that the result is the milliseconds from the earlier to the later time point.");

    const std::chrono::steady_clock::time_point earlier{100s};

    EXPECT_EQ(ElapsedMs(earlier, earlier), 0U);
    EXPECT_EQ(ElapsedMs(earlier, earlier + 1500ms), 1500U);
}

TEST(ElapsedMsTest, TruncatesToWholeMilliseconds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_granularity");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that 80 ms - 1 us counts as 79 ms.");

    const std::chrono::steady_clock::time_point earlier{100s};

    EXPECT_EQ(ElapsedMs(earlier, earlier + 80ms - 1us), 79U);
}

TEST(ElapsedMsTest, LaterTimePointBeforeEarlierCountsAsZero)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monotonic_time");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a later time point before the earlier one gives 0 and does not wrap.");

    const std::chrono::steady_clock::time_point earlier{100s};

    EXPECT_EQ(ElapsedMs(earlier, earlier - 1ns), 0U);
    EXPECT_EQ(ElapsedMs(earlier, earlier - 5ms), 0U);
}

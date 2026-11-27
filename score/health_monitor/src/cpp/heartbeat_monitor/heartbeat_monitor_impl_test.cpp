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

#include "score/health_monitor/src/cpp/heartbeat_monitor/heartbeat_monitor_impl.h"
#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/fake_monotonic_time.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/mw/log/recorder_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <memory>
#include <string_view>
#include <tuple>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{

using Monitor = HeartbeatMonitorImpl;
using TimePoint = std::chrono::steady_clock::time_point;

const MonitorTag kTag{"heartbeat"};
// Starts at 100 s so that a time before creation is still a non-negative timespec.
const TimePoint kCreation{100s};

}  // namespace

TEST(HeartbeatMonitorImplCreateTest, AcceptsCycleShorterThanTwiceRangeMin)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a cycle one millisecond below twice the range minimum is accepted.");

    const TimePoint now{kCreation};
    StrictMock<score::os::TimeMock> time;
    ReportMonotonicTime(time, now);

    const auto result = Monitor::Create(kTag, TimeRange{500ms, 1000ms}, 999ms, time);

    EXPECT_TRUE(result.has_value());
}

TEST(HeartbeatMonitorImplCreateTest, RejectsCycleEqualToTwiceRangeMin)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a cycle equal to twice the range minimum fails with kInvalidArgument.");

    const TimePoint now{kCreation};
    StrictMock<score::os::TimeMock> time;
    ReportMonotonicTime(time, now);

    const auto result = Monitor::Create(kTag, TimeRange{500ms, 1000ms}, 1000ms, time);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kInvalidArgument));
}

TEST(HeartbeatMonitorImplCreateTest, RejectsCycleLongerThanTwiceRangeMin)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a cycle above twice the range minimum fails with kInvalidArgument.");

    const TimePoint now{kCreation};
    StrictMock<score::os::TimeMock> time;
    ReportMonotonicTime(time, now);

    const auto result = Monitor::Create(kTag, TimeRange{500ms, 1000ms}, 1001ms, time);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kInvalidArgument));
}

TEST(HeartbeatMonitorImplCreateTest, AcceptsLargestRangeMin)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the largest range minimum is accepted without an overflow.");

    const TimePoint now{kCreation};
    StrictMock<score::os::TimeMock> time;
    ReportMonotonicTime(time, now);
    const auto largest = std::chrono::milliseconds::max();

    const auto result = Monitor::Create(kTag, TimeRange{largest - 1ms, largest}, 1ms, time);

    EXPECT_TRUE(result.has_value());
}

TEST(HeartbeatMonitorImplCreateTest, ClockReadFailureAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a failed CLOCK_MONOTONIC read at creation aborts.");

    StrictMock<score::os::TimeMock> time;
    EXPECT_CALL(time, clock_gettime(CLOCK_MONOTONIC, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    EXPECT_EXIT(
        std::ignore = Monitor::Create(kTag, TimeRange{500ms, 1000ms}, 999ms, time),
        ::testing::KilledBySignal(SIGABRT),
        "");
}

/// @brief Monitor with range [80, 120] ms, created at `kCreation`, with the worker started at the same time.
/// Every time in the tests is an offset from `kCreation`.
class HeartbeatMonitorImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ReportMonotonicTime(time_, now_);
        monitor_ = Monitor::Create(kTag, TimeRange{80ms, 120ms}, 1ms, time_).value();
    }

    void BeatAt(const std::chrono::steady_clock::duration time, const int count = 1)
    {
        now_ = kCreation + time;
        for (int i{0}; i < count; ++i)
        {
            monitor_->Heartbeat();
        }
    }

    /// @brief Evaluates the monitor at `time` and returns `false` if it failed.
    bool EvaluateAt(const std::chrono::steady_clock::duration time)
    {
        now_ = kCreation + time;
        return monitor_->Evaluate(hmon_start_);
    }

    TimePoint now_{kCreation};
    TimePoint hmon_start_{kCreation};
    StrictMock<score::os::TimeMock> time_;
    std::shared_ptr<Monitor> monitor_;
};

TEST_F(HeartbeatMonitorImplTest, HeartbeatInRangeIsAccepted)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a heartbeat inside the range passes.");

    BeatAt(100ms);

    EXPECT_TRUE(EvaluateAt(100ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatAtRangeMinIsAccepted)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat exactly at the range minimum passes.");

    BeatAt(80ms);

    EXPECT_TRUE(EvaluateAt(80ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatAtRangeMaxIsAccepted)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_late, comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat exactly at the range maximum passes.");

    BeatAt(120ms);

    EXPECT_TRUE(EvaluateAt(120ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatBeforeRangeIsTooEarly)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat one millisecond before the range is too early.");

    BeatAt(79ms);

    EXPECT_FALSE(EvaluateAt(100ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatAfterRangeIsTooLate)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_late, comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat one millisecond after the range is too late.");

    BeatAt(121ms);

    EXPECT_FALSE(EvaluateAt(121ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatTimeIsTruncatedToWholeMilliseconds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_granularity");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat at 80 ms - 1 us truncates to 79 ms and is too early.");

    BeatAt(80ms - 1us);

    EXPECT_FALSE(EvaluateAt(100ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatAfterFractionalWorkerStartIsTooLate)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__time_granularity, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a worker start at 0.5 ms does not delay a missing heartbeat past the range maximum.");

    hmon_start_ = kCreation + 500us;
    BeatAt(100ms);
    ASSERT_TRUE(EvaluateAt(100ms));

    EXPECT_TRUE(EvaluateAt(220ms));
    EXPECT_FALSE(EvaluateAt(221ms));
}

TEST_F(HeartbeatMonitorImplTest, InRangeHeartbeatEvaluatedAfterRangeIsAccepted)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a heartbeat inside the range passes when evaluated after the range.");

    BeatAt(90ms);

    EXPECT_TRUE(EvaluateAt(150ms));
}

TEST_F(HeartbeatMonitorImplTest, EarlyHeartbeatEvaluatedAfterRangeIsTooEarly)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_early");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that an early heartbeat is too early when evaluated after the range.");

    BeatAt(25ms);

    EXPECT_FALSE(EvaluateAt(150ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatIsAcceptedUntilRangeMax)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_late, comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a missing heartbeat passes up to the range maximum.");

    EXPECT_TRUE(EvaluateAt(0ms));
    EXPECT_TRUE(EvaluateAt(100ms));
    EXPECT_TRUE(EvaluateAt(120ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatAfterRangeMaxIsTooLate)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_late, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a missing heartbeat is too late one millisecond after the range.");

    EXPECT_FALSE(EvaluateAt(121ms));
}

TEST_F(HeartbeatMonitorImplTest, TwoHeartbeatsInRangeInOneEvaluationAreMultiple)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that two in-range heartbeats in one cycle fail.");

    BeatAt(90ms);
    BeatAt(100ms);

    EXPECT_FALSE(EvaluateAt(100ms));
}

TEST_F(HeartbeatMonitorImplTest, PassingEvaluationsDoNotLog)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that an evaluation without a heartbeat before the range maximum and one with a heartbeat "
        "inside the range log nothing.");

    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    EXPECT_CALL(recorder, StartRecord(_, _)).Times(0);
    const ScopedLogRecorder scoped_recorder{recorder};

    ASSERT_TRUE(EvaluateAt(50ms));
    BeatAt(100ms);
    EXPECT_TRUE(EvaluateAt(100ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatDuringAFailureLogCountsInTheNextEvaluation)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__failure_log, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that a heartbeat recorded while a too-early heartbeat is logged counts in the next "
        "evaluation.");

    BeatAt(79ms);
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    bool beat_during_log{false};
    // The recorder call stands in for a log backend that blocks while the application sends its next heartbeat.
    EXPECT_CALL(recorder, StartRecord(_, score::mw::log::LogLevel::kWarn))
        .WillOnce([this, &beat_during_log](const std::string_view, const score::mw::log::LogLevel) {
            monitor_->Heartbeat();
            beat_during_log = true;
            return score::cpp::optional<score::mw::log::SlotHandle>{};
        });
    {
        const ScopedLogRecorder scoped_recorder{recorder};
        ASSERT_FALSE(EvaluateAt(100ms));
    }

    ASSERT_TRUE(beat_during_log);

    // The heartbeat at 100 ms is inside [80, 120] ms. A lost heartbeat would fail at 121 ms as missing.
    EXPECT_TRUE(EvaluateAt(121ms));
}

TEST_F(HeartbeatMonitorImplTest, AcceptedHeartbeatStartsNextCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_early");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the next range starts at an accepted heartbeat.");

    BeatAt(100ms);
    ASSERT_TRUE(EvaluateAt(100ms));

    BeatAt(179ms);

    EXPECT_FALSE(EvaluateAt(179ms));
}

TEST_F(HeartbeatMonitorImplTest, NextCycleRangeIsMeasuredFromAcceptedHeartbeat)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that each accepted heartbeat moves the range of the next cycle.");

    BeatAt(100ms);
    ASSERT_TRUE(EvaluateAt(100ms));
    BeatAt(180ms);
    ASSERT_TRUE(EvaluateAt(180ms));

    BeatAt(300ms);

    EXPECT_TRUE(EvaluateAt(300ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatIsMeasuredFromAcceptedHeartbeat)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that a missing heartbeat is too late after the range of the last accepted one.");

    BeatAt(100ms);
    ASSERT_TRUE(EvaluateAt(100ms));

    EXPECT_TRUE(EvaluateAt(220ms));
    EXPECT_FALSE(EvaluateAt(221ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatAfterRangeOfLaterCycleIsTooLate)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a heartbeat after the range of a later cycle is too late.");

    BeatAt(100ms);
    ASSERT_TRUE(EvaluateAt(100ms));

    BeatAt(221ms);

    EXPECT_FALSE(EvaluateAt(221ms));
}

TEST_F(HeartbeatMonitorImplTest, MultipleHeartbeatsDoNotStartNextCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that multiple heartbeats keep the range of the current cycle.");

    BeatAt(90ms, 2);
    ASSERT_FALSE(EvaluateAt(100ms));

    EXPECT_TRUE(EvaluateAt(120ms));
    EXPECT_FALSE(EvaluateAt(121ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatsAreCountedAgainAfterEvaluation)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that each evaluation starts counting heartbeats from zero.");

    BeatAt(90ms, 2);
    ASSERT_FALSE(EvaluateAt(100ms));

    BeatAt(110ms);

    EXPECT_TRUE(EvaluateAt(110ms));
}

TEST_F(HeartbeatMonitorImplTest, FailedHeartbeatDoesNotStartNextCycle)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a too early heartbeat keeps the range of the current cycle.");

    BeatAt(25ms);
    ASSERT_FALSE(EvaluateAt(50ms));

    EXPECT_TRUE(EvaluateAt(120ms));
    EXPECT_FALSE(EvaluateAt(121ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatKeepsFailingInLaterEvaluations)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a missing heartbeat stays too late in later evaluations.");

    ASSERT_FALSE(EvaluateAt(121ms));

    EXPECT_FALSE(EvaluateAt(200ms));
}

TEST_F(HeartbeatMonitorImplTest, RegularHeartbeatsNeverFail)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a heartbeat every 100 ms inside the range never fails.");

    for (auto time{20ms}; time <= 1000ms; time += 20ms)
    {
        if (time % 100ms == 0ms)
        {
            BeatAt(time);
        }

        ASSERT_TRUE(EvaluateAt(time)) << "at " << time.count() << " ms";
    }
}

TEST_F(HeartbeatMonitorImplTest, FirstCycleStartsAtWorkerStart)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__hb_too_early, comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the first range starts when the worker starts.");

    hmon_start_ = kCreation + 300ms;

    BeatAt(390ms);

    EXPECT_TRUE(EvaluateAt(400ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatBeforeRangeOfFirstCycleIsTooEarly)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_early");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a heartbeat before the first range after worker start is too early.");

    hmon_start_ = kCreation + 300ms;

    BeatAt(350ms);

    EXPECT_FALSE(EvaluateAt(400ms));
}

TEST_F(HeartbeatMonitorImplTest, HeartbeatBeforeWorkerStartCountsInFirstCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_early");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a heartbeat before worker start counts in the first cycle.");

    BeatAt(250ms);
    hmon_start_ = kCreation + 300ms;

    EXPECT_FALSE(EvaluateAt(310ms));
}

TEST_F(HeartbeatMonitorImplTest, MissingHeartbeatInFirstCycleIsMeasuredFromWorkerStart)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a missing first heartbeat is too late after worker start plus maximum.");

    hmon_start_ = kCreation + 300ms;

    EXPECT_TRUE(EvaluateAt(420ms));
    EXPECT_FALSE(EvaluateAt(421ms));
}

TEST_F(HeartbeatMonitorImplTest, EvaluationWithClockBeforeWorkerStartCountsAtWorkerStart)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that an evaluation at a time before worker start counts at worker start.");

    hmon_start_ = kCreation + 300ms;

    EXPECT_TRUE(EvaluateAt(290ms));
}

TEST_F(HeartbeatMonitorImplTest, EvaluateWithWorkerStartBeforeCreationAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that an evaluation with worker start before monitor creation aborts.");

    EXPECT_DEATH(std::ignore = monitor_->Evaluate(kCreation - 1ms), "");
}

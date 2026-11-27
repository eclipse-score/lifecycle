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

#include "score/health_monitor/src/cpp/internal/monitoring_logic.h"

#include "score/health_monitor/src/cpp/internal/fake_monotonic_time.h"
#include "score/health_monitor/src/cpp/internal/mock_supervisor_client.h"
#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/mw/log/recorder_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <string_view>
#include <vector>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{

class MonitorEvaluatorMock : public MonitorEvaluator
{
  public:
    MOCK_METHOD(bool, Evaluate, (const std::chrono::steady_clock::time_point hmon_start), (noexcept, override));
};

/// @brief Logic on a clock the test advances, over a set of evaluator mocks.
class MonitoringLogicTest : public ::testing::Test
{
  protected:
    MonitoringLogicTest()
    {
        ReportMonotonicTime(time_, now_);
        // The gmock default `false` would be a failure, so a monitor passes unless a test says otherwise.
        for (auto& evaluator : evaluators_)
        {
            ON_CALL(evaluator, Evaluate(_)).WillByDefault(Return(true));
        }
    }

    /// @brief Builds the logic at the current fake time, so `last_notification` is that time.
    MonitoringLogic MakeLogic(const std::chrono::milliseconds supervisor_api_cycle)
    {
        evaluator_pointers_.clear();
        for (auto& evaluator : evaluators_)
        {
            evaluator_pointers_.push_back(&evaluator);
        }
        return MonitoringLogic{
            score::cpp::span<MonitorEvaluator* const>{evaluator_pointers_}, supervisor_api_cycle, client_, time_};
    }

    void Advance(const std::chrono::steady_clock::duration step)
    {
        now_ += step;
    }

    std::chrono::steady_clock::time_point now_{std::chrono::steady_clock::time_point{} + 1000s};
    const std::chrono::steady_clock::time_point hmon_start_{now_};
    StrictMock<score::os::TimeMock> time_;
    StrictMock<MonitorEvaluatorMock> evaluators_[3];
    StrictMock<SupervisorClientMock> client_;
    std::vector<MonitorEvaluator*> evaluator_pointers_;
};

}  // namespace

TEST_F(MonitoringLogicTest, EvaluatesEveryMonitorOncePerRun)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that each run calls Evaluate once on every monitor.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }
    auto logic = MakeLogic(500ms);

    logic.Run(hmon_start_);
}

TEST_F(MonitoringLogicTest, PassesTheWorkerStartToTheMonitors)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Run passes the given worker start time to every monitor.");

    const auto start = hmon_start_ + 7ms;
    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(start)).Times(1);
    }
    auto logic = MakeLogic(500ms);

    logic.Run(start);
}

TEST_F(MonitoringLogicTest, EvaluatesEveryMonitorAfterAnError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a monitor that reports an error does not stop the evaluation of the others.");

    EXPECT_CALL(evaluators_[0], Evaluate(_)).WillOnce(Return(false));
    EXPECT_CALL(evaluators_[1], Evaluate(_)).Times(1);
    EXPECT_CALL(evaluators_[2], Evaluate(_)).Times(1);
    auto logic = MakeLogic(500ms);

    logic.Run(hmon_start_);
}

TEST_F(MonitoringLogicTest, EvaluatesEveryMonitorAfterAnErrorOfTheLastMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that every monitor is evaluated when only the last one reports an error.");

    EXPECT_CALL(evaluators_[0], Evaluate(_)).Times(1);
    EXPECT_CALL(evaluators_[1], Evaluate(_)).Times(1);
    EXPECT_CALL(evaluators_[2], Evaluate(_)).WillOnce(Return(false));
    auto logic = MakeLogic(500ms);

    EXPECT_FALSE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, ReturnsTrueWhenNoMonitorReportsAnError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that Run returns true when every monitor is healthy.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }
    auto logic = MakeLogic(500ms);

    EXPECT_TRUE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, ErrorReturnsFalseAndWithholdsAliveNotification)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_withhold");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that a run with an error returns false and does not notify alive although the supervisor "
        "cycle has passed.");

    EXPECT_CALL(evaluators_[0], Evaluate(_)).WillOnce(Return(false));
    EXPECT_CALL(evaluators_[1], Evaluate(_)).Times(1);
    EXPECT_CALL(evaluators_[2], Evaluate(_)).Times(1);
    EXPECT_CALL(client_, NotifyAlive()).Times(0);
    auto logic = MakeLogic(30ms);
    Advance(1000ms);

    EXPECT_FALSE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, WithholdsAliveWhileTheFailureIsLogged)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__failure_log, comp_req__health_monitor__alive_withhold");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that a run with an error logs a warning and does not notify alive, also when a supervisor "
        "cycle passes while the warning is logged.");

    EXPECT_CALL(evaluators_[0], Evaluate(_)).WillOnce(Return(false));
    EXPECT_CALL(evaluators_[1], Evaluate(_)).Times(1);
    EXPECT_CALL(evaluators_[2], Evaluate(_)).Times(1);
    EXPECT_CALL(client_, NotifyAlive()).Times(0);
    auto logic = MakeLogic(30ms);
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    // The supervisor cycle passes while the warning is logged, so a notification check after the log would notify.
    EXPECT_CALL(recorder, StartRecord(_, score::mw::log::LogLevel::kWarn))
        .WillOnce([this](const std::string_view, const score::mw::log::LogLevel) {
            Advance(1000ms);
            return score::cpp::optional<score::mw::log::SlotHandle>{};
        });
    const ScopedLogRecorder scoped_recorder{recorder};

    EXPECT_FALSE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, PassingRunDoesNotLog)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__detect_latency, comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a run without an error notifies alive and logs nothing.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }

    EXPECT_CALL(client_, NotifyAlive()).Times(1);
    auto logic = MakeLogic(30ms);
    Advance(1000ms);
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    EXPECT_CALL(recorder, StartRecord(_, _)).Times(0);
    const ScopedLogRecorder scoped_recorder{recorder};

    EXPECT_TRUE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, NotifiesAliveOnEveryRunWithZeroSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a supervisor cycle of zero notifies alive on each run when the clock advanced by one tick.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(3);
    }
    EXPECT_CALL(client_, NotifyAlive()).Times(3);
    auto logic = MakeLogic(0ms);

    for (int run{0}; run < 3; ++run)
    {
        Advance(std::chrono::steady_clock::duration{1});
        EXPECT_TRUE(logic.Run(hmon_start_));
    }
}

TEST_F(MonitoringLogicTest, DoesNotNotifyAliveWithZeroSupervisorCycleWhenTheClockStands)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a clock that does not advance does not notify alive even with a supervisor cycle of zero.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }
    EXPECT_CALL(client_, NotifyAlive()).Times(0);
    auto logic = MakeLogic(0ms);

    EXPECT_TRUE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, DoesNotNotifyAliveWhenElapsedTimeEqualsSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that an elapsed time of exactly one supervisor cycle does not notify alive.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }
    EXPECT_CALL(client_, NotifyAlive()).Times(0);
    auto logic = MakeLogic(30ms);
    Advance(30ms);

    EXPECT_TRUE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, NotifiesAliveWhenElapsedTimeExceedsSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that an elapsed time of one millisecond above the supervisor cycle notifies alive once.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(1);
    }
    EXPECT_CALL(client_, NotifyAlive()).Times(1);
    auto logic = MakeLogic(30ms);
    Advance(31ms);

    EXPECT_TRUE(logic.Run(hmon_start_));
}

TEST_F(MonitoringLogicTest, NotificationRestartsTheSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that the supervisor cycle is measured from the last notification and not from the construction.");

    for (auto& evaluator : evaluators_)
    {
        EXPECT_CALL(evaluator, Evaluate(_)).Times(3);
    }
    EXPECT_CALL(client_, NotifyAlive()).Times(2);
    auto logic = MakeLogic(30ms);

    Advance(31ms);
    EXPECT_TRUE(logic.Run(hmon_start_));  // notifies
    Advance(30ms);
    EXPECT_TRUE(logic.Run(hmon_start_));  // 30 ms since the notification: none
    Advance(1ms);
    EXPECT_TRUE(logic.Run(hmon_start_));  // 31 ms: notifies
}

TEST_F(MonitoringLogicTest, NotifiesAliveAgainAfterAnErrorFreeRunFollowingAFailedOne)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__alive_withhold");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that a failed run does not restart the supervisor cycle, so the next healthy run notifies alive.");

    EXPECT_CALL(evaluators_[0], Evaluate(_)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(evaluators_[1], Evaluate(_)).Times(2);
    EXPECT_CALL(evaluators_[2], Evaluate(_)).Times(2);
    EXPECT_CALL(client_, NotifyAlive()).Times(1);
    auto logic = MakeLogic(30ms);

    Advance(31ms);
    EXPECT_FALSE(logic.Run(hmon_start_));
    Advance(1ms);
    EXPECT_TRUE(logic.Run(hmon_start_));
}

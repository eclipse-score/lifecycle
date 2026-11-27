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

#include "score/health_monitor/src/cpp/deadline_monitor/deadline_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/fake_monotonic_time.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/mw/log/logging.h"
#include "score/mw/log/recorder_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;
using ::testing::StrictMock;

namespace
{

using Monitor = DeadlineMonitorImpl;
using TimePoint = std::chrono::steady_clock::time_point;

const MonitorTag kTag{"deadlines"};
const DeadlineTag kA{"a"};
const DeadlineTag kB{"b"};
const DeadlineTag kC{"c"};
const DeadlineTag kUnknown{"unknown"};
// Starts at 100 s so that a time before creation is still a non-negative timespec.
const TimePoint kCreation{100s};
// Largest deadline timestamp and largest range maximum.
constexpr std::int64_t kLargest32BitMs{std::numeric_limits<std::uint32_t>::max()};
// Start of the deadline "a" whose expected end is the largest timestamp.
constexpr std::chrono::milliseconds kLastStart{kLargest32BitMs - 120};

/// @brief Routes every log message to `recorder` until destroyed, then restores the default recorder.
class ScopedLogRecorder final
{
  public:
    explicit ScopedLogRecorder(score::mw::log::Recorder& recorder) noexcept
    {
        score::mw::log::SetLogRecorder(&recorder);
    }
    ScopedLogRecorder(const ScopedLogRecorder&) = delete;
    ScopedLogRecorder(ScopedLogRecorder&&) = delete;
    ScopedLogRecorder& operator=(const ScopedLogRecorder&) = delete;
    ScopedLogRecorder& operator=(ScopedLogRecorder&&) = delete;
    ~ScopedLogRecorder() noexcept
    {
        score::mw::log::SetLogRecorder(nullptr);
    }
};

/// @brief Monitor "deadlines" created at `kCreation`, whose clock is `now_`.
/// Every time in the tests is an offset from `kCreation`.
/// The deadline "a" has the range [80, 120] ms.
class DeadlineMonitorImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ReportMonotonicTime(time_, now_);
    }

    static std::vector<DeadlineConfig> Single()
    {
        std::vector<DeadlineConfig> deadlines;
        deadlines.push_back(DeadlineConfig{kA, TimeRange{80ms, 120ms}});
        return deadlines;
    }

    score::Result<std::shared_ptr<Monitor>> TryCreate(std::vector<DeadlineConfig>&& deadlines)
    {
        return Monitor::Create(kTag, std::move(deadlines), time_);
    }

    void CreateMonitor(std::vector<DeadlineConfig>&& deadlines)
    {
        auto result = TryCreate(std::move(deadlines));
        EXPECT_TRUE(result.has_value());
        if (result.has_value())
        {
            monitor_ = std::move(result).value();
        }
    }

    /// @brief Returns the taken deadline, or `nullptr` if the monitor is missing or refuses it.
    Deadline* Take(const DeadlineTag& tag)
    {
        if (monitor_ == nullptr)
        {
            return nullptr;
        }
        auto result = monitor_->GetDeadline(tag);
        if (!result.has_value())
        {
            return nullptr;
        }
        taken_.push_back(std::move(result).value());
        return &taken_.back().Get();
    }

    DeadlineHandle StartAt(Deadline& deadline, const std::chrono::steady_clock::duration time)
    {
        now_ = kCreation + time;
        auto result = deadline.Start();
        EXPECT_TRUE(result.has_value());
        return std::move(result).value();
    }

    void StopAt(DeadlineHandle& handle, const std::chrono::steady_clock::duration time)
    {
        now_ = kCreation + time;
        handle.Stop();
    }

    /// @brief Evaluates the monitor at `time` and returns `false` if it failed.
    bool EvaluateAt(const std::chrono::steady_clock::duration time)
    {
        now_ = kCreation + time;
        return monitor_->Evaluate(kCreation);
    }

    TimePoint now_{kCreation};
    StrictMock<score::os::TimeMock> time_;
    std::shared_ptr<Monitor> monitor_;
    std::vector<Taken<Deadline>> taken_;
};

using DeadlineMonitorImplDeathTest = DeadlineMonitorImplTest;

}  // namespace

TEST_F(DeadlineMonitorImplTest, CreatesMonitorWithOneDeadline)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that one deadline creates a monitor.");

    const auto result = TryCreate(Single());

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(DeadlineMonitorImplTest, RejectsNoDeadlines)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that an empty deadline list fails with kInvalidArgument.");

    ExpectError(TryCreate(std::vector<DeadlineConfig>{}), ErrorCode::kInvalidArgument);
}

TEST_F(DeadlineMonitorImplTest, AcceptsDistinctDeadlineTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that three deadlines with different tags create a monitor.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kC, TimeRange{1ms, 2ms}});

    EXPECT_TRUE(TryCreate(std::move(deadlines)).has_value());
}

TEST_F(DeadlineMonitorImplTest, RejectsRepeatedDeadlineTagWithDifferentRanges)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two deadlines with one tag and different ranges fail.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kA, TimeRange{5ms, 9ms}});

    ExpectError(TryCreate(std::move(deadlines)), ErrorCode::kInvalidArgument);
}

TEST_F(DeadlineMonitorImplTest, RejectsRepeatedDeadlineTagWithEqualRanges)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two identical deadlines fail with kInvalidArgument.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kA, TimeRange{1ms, 2ms}});

    ExpectError(TryCreate(std::move(deadlines)), ErrorCode::kInvalidArgument);
}

TEST_F(DeadlineMonitorImplTest, AcceptsRangeMaximumOf32Bits)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a range maximum of 2^32 - 1 ms is accepted.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{0ms, std::chrono::milliseconds{kLargest32BitMs}}});

    EXPECT_TRUE(TryCreate(std::move(deadlines)).has_value());
}

TEST_F(DeadlineMonitorImplTest, RejectsRangeMaximumAbove32Bits)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a range maximum of 2^32 ms fails with kInvalidArgument.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{0ms, std::chrono::milliseconds{kLargest32BitMs + 1}}});

    ExpectError(TryCreate(std::move(deadlines)), ErrorCode::kInvalidArgument);
}

TEST_F(DeadlineMonitorImplTest, RejectsRangeMaximumAbove32BitsOfALaterDeadline)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a range maximum of 2^32 ms fails with kInvalidArgument also after a valid deadline.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{80ms, 120ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{0ms, std::chrono::milliseconds{kLargest32BitMs + 1}}});

    ExpectError(TryCreate(std::move(deadlines)), ErrorCode::kInvalidArgument);
}

TEST_F(DeadlineMonitorImplTest, ConfiguredDeadlineCanBeTaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetDeadline returns a configured deadline.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);

    const auto result = monitor_->GetDeadline(kA);

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(&result.value().Get(), nullptr);
}

TEST_F(DeadlineMonitorImplTest, SecondTakeOfTheSameDeadlineFailsWithAlreadyTaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that taking a deadline twice fails with kAlreadyTaken.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    const auto first = monitor_->GetDeadline(kA);
    ASSERT_TRUE(first.has_value());

    ExpectError(monitor_->GetDeadline(kA), ErrorCode::kAlreadyTaken);
}

TEST_F(DeadlineMonitorImplTest, ReleasedDeadlineCanBeTakenAgain)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a deadline can be taken again after its handle is destroyed.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    {
        const auto first = monitor_->GetDeadline(kA);
        ASSERT_TRUE(first.has_value());
    }

    const auto second = monitor_->GetDeadline(kA);

    EXPECT_TRUE(second.has_value());
}

TEST_F(DeadlineMonitorImplTest, DeadlineStaysTakenWhileAMovedHandleHoldsIt)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that destroying a moved-from handle keeps the deadline taken and destroying the moved-to "
        "handle releases it.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    auto result = monitor_->GetDeadline(kA);
    ASSERT_TRUE(result.has_value());
    std::optional<Taken<Deadline>> target;
    {
        Taken<Deadline> source{std::move(result).value()};
        target.emplace(std::move(source));
    }

    ExpectError(monitor_->GetDeadline(kA), ErrorCode::kAlreadyTaken);

    target.reset();

    EXPECT_TRUE(monitor_->GetDeadline(kA).has_value());
}

TEST_F(DeadlineMonitorImplTest, RetakenDeadlineKeepsItsFailure)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive, comp_req__health_monitor__dl_too_early");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that a deadline stopped too early still fails Start and Evaluate after it is released and "
        "taken again.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    {
        auto first = monitor_->GetDeadline(kA);
        ASSERT_TRUE(first.has_value());
        auto handle = StartAt(*first.value(), 10ms);
        StopAt(handle, 20ms);
    }

    auto second = monitor_->GetDeadline(kA);

    ASSERT_TRUE(second.has_value());
    ExpectError(second.value()->Start(), ErrorCode::kDeadlineFailed);
    EXPECT_FALSE(EvaluateAt(30ms));
}

TEST_F(DeadlineMonitorImplTest, ConcurrentTakersNeverHoldTheDeadlineTogether)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that two threads that take and release one deadline in a loop never hold it at the same "
        "time and both take it.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    constexpr int kAttempts{5000};
    std::atomic<int> holders{0};
    std::atomic<bool> overlapped{false};
    const auto take_and_release = [this, &holders, &overlapped](int& takes) {
        for (int i{0}; i < kAttempts; ++i)
        {
            auto result = monitor_->GetDeadline(kA);
            if (!result.has_value())
            {
                continue;
            }
            ++takes;
            if (holders.fetch_add(1) != 0)
            {
                overlapped = true;
            }
            holders.fetch_sub(1);
        }
    };
    int first_takes{0};
    int second_takes{0};

    std::thread other{[&take_and_release, &second_takes] {
        take_and_release(second_takes);
    }};
    take_and_release(first_takes);
    other.join();

    EXPECT_FALSE(overlapped.load());
    EXPECT_GT(first_takes + second_takes, 1);
}

TEST_F(DeadlineMonitorImplTest, UnknownDeadlineTagFailsWithUnknownTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetDeadline with an unconfigured tag fails with kUnknownTag.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);

    ExpectError(monitor_->GetDeadline(kUnknown), ErrorCode::kUnknownTag);
}

TEST_F(DeadlineMonitorImplTest, TakingOneDeadlineDoesNotTakeTheOther)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_exclusive");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two deadlines of one monitor can both be taken and are different.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{1ms, 2ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{1ms, 2ms}});
    CreateMonitor(std::move(deadlines));
    ASSERT_NE(monitor_, nullptr);
    const auto first = monitor_->GetDeadline(kA);
    ASSERT_TRUE(first.has_value());

    const auto second = monitor_->GetDeadline(kB);

    ASSERT_TRUE(second.has_value());
    EXPECT_NE(&first.value().Get(), &second.value().Get());
}

TEST_F(DeadlineMonitorImplTest, StopInsideTheRangePasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stop between the bounds passes and the deadline starts again.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 110ms);

    EXPECT_TRUE(EvaluateAt(110ms));
    auto second = StartAt(*deadline, 110ms);
    EXPECT_TRUE(EvaluateAt(110ms));
    StopAt(second, 200ms);
    EXPECT_TRUE(EvaluateAt(200ms));
}

TEST_F(DeadlineMonitorImplTest, StopAtExactlyTheMinimumPasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stop exactly at the range minimum passes and the deadline restarts.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 90ms);

    EXPECT_TRUE(EvaluateAt(90ms));
    auto second = StartAt(*deadline, 90ms);
    EXPECT_TRUE(EvaluateAt(90ms));
    StopAt(second, 200ms);
    EXPECT_TRUE(EvaluateAt(200ms));
}

TEST_F(DeadlineMonitorImplTest, StopAtTheStartPassesForAZeroMinimum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stop in the start millisecond passes for a range minimum of 0 ms.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{0ms, 120ms}});
    CreateMonitor(std::move(deadlines));
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 10ms);

    EXPECT_TRUE(EvaluateAt(10ms));
}

TEST_F(DeadlineMonitorImplTest, StopDuringTheClockReadOfEvaluatePasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that a stop at the maximum during the clock read of Evaluate is no missed deadline.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);
    // The first clock read of Evaluate stops the deadline at its maximum, then reports 1 ms past it.
    bool stop_on_read{true};
    EXPECT_CALL(time_, clock_gettime(CLOCK_MONOTONIC, ::testing::_))
        .WillRepeatedly([this, &handle, &stop_on_read](const clockid_t, timespec* const tp) {
            if (stop_on_read)
            {
                stop_on_read = false;
                StopAt(handle, 130ms);
                now_ = kCreation + 131ms;
            }
            *tp = ToTimespec(now_);
            return score::cpp::expected<std::int32_t, score::os::Error>{0};
        });

    EXPECT_TRUE(monitor_->Evaluate(kCreation));
    EXPECT_FALSE(stop_on_read);
}

TEST_F(DeadlineMonitorImplTest, StopAtExactlyTheMaximumPasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__time_range_bounds");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stop exactly at the range maximum passes and the deadline restarts.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 130ms);

    EXPECT_TRUE(EvaluateAt(130ms));
    auto second = StartAt(*deadline, 130ms);
    EXPECT_TRUE(EvaluateAt(130ms));
    StopAt(second, 220ms);
    EXPECT_TRUE(EvaluateAt(220ms));
}

TEST_F(DeadlineMonitorImplTest, StopOneMillisecondBeforeTheMinimumFails)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_too_early, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a stop 1 ms before the range minimum makes Evaluate return false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 89ms);

    EXPECT_FALSE(EvaluateAt(89ms));
}

TEST_F(DeadlineMonitorImplTest, StopImmediatelyAfterStartFails)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_too_early, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a stop in the same millisecond as the start makes Evaluate return false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 10ms);

    EXPECT_FALSE(EvaluateAt(10ms));
}

TEST_F(DeadlineMonitorImplTest, EarlyStopAfterAnEvaluationInsideTheRangeFails)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_too_early, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that Evaluate returns true while the deadline runs inside its range and false after an "
        "early stop.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);
    EXPECT_TRUE(EvaluateAt(50ms));

    StopAt(handle, 60ms);

    EXPECT_FALSE(EvaluateAt(60ms));
}

TEST_F(DeadlineMonitorImplTest, StopOneMillisecondAfterTheMaximumFails)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a stop 1 ms after the range maximum makes Evaluate return false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    StopAt(handle, 131ms);

    EXPECT_FALSE(EvaluateAt(131ms));
}

TEST_F(DeadlineMonitorImplTest, LateStopStillFailsInALaterEvaluation)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a late stop makes every later Evaluate return false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);
    StopAt(handle, 500ms);

    EXPECT_FALSE(EvaluateAt(500ms));
    EXPECT_FALSE(EvaluateAt(900ms));
}

TEST_F(DeadlineMonitorImplTest, StartThatEndsAtTheLargestTimestampPasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a deadline whose expected end is 2^32 - 1 ms starts and stops in time.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, kLastStart);

    StopAt(handle, kLastStart + 100ms);

    EXPECT_TRUE(EvaluateAt(kLastStart + 100ms));
}

TEST_F(DeadlineMonitorImplDeathTest, StartThatEndsAfterTheLargestTimestampAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a start whose expected end is 2^32 ms terminates the process.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);

    EXPECT_DEATH({ auto handle = StartAt(*deadline, kLastStart + 1ms); }, "");
}

TEST_F(DeadlineMonitorImplTest, RunningDeadlinePastTheLargestTimestampFails)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that Evaluate at 2^32 ms returns false for a deadline that ends at 2^32 - 1 ms.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, kLastStart);

    EXPECT_FALSE(EvaluateAt(kLastStart + 121ms));
}

TEST_F(DeadlineMonitorImplTest, StopPastTheLargestTimestampFails)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stop at 2^32 ms of a deadline that ends at 2^32 - 1 ms fails.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, kLastStart);

    StopAt(handle, kLastStart + 121ms);

    EXPECT_FALSE(EvaluateAt(kLastStart + 121ms));
}

TEST_F(DeadlineMonitorImplTest, DeadlineThatNeverStartedPasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that Evaluate returns true for a deadline that was never started.");

    CreateMonitor(Single());
    ASSERT_NE(Take(kA), nullptr);

    EXPECT_TRUE(EvaluateAt(0ms));
    EXPECT_TRUE(EvaluateAt(10s));
}

TEST_F(DeadlineMonitorImplTest, RunningDeadlineAtExactlyTheMaximumPasses)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that Evaluate exactly at start plus range maximum returns true.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    EXPECT_TRUE(EvaluateAt(130ms));

    StopAt(handle, 130ms);
}

TEST_F(DeadlineMonitorImplTest, RunningDeadlineOneMillisecondPastTheMaximumFails)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that Evaluate 1 ms after start plus range maximum returns false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    EXPECT_FALSE(EvaluateAt(131ms));
}

TEST_F(DeadlineMonitorImplTest, RunningDeadlinePastTheMaximumStillFailsInALaterEvaluation)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__failure_latch");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate keeps returning false for a deadline that never stops.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    EXPECT_FALSE(EvaluateAt(200ms));
    EXPECT_FALSE(EvaluateAt(300ms));
}

TEST_F(DeadlineMonitorImplTest, ThreeRunningDeadlinesPastTheirMaximaFail)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate returns false when three started deadlines all missed.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{10ms, 20ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{10ms, 30ms}});
    deadlines.push_back(DeadlineConfig{kC, TimeRange{10ms, 40ms}});
    CreateMonitor(std::move(deadlines));
    Deadline* const a{Take(kA)};
    Deadline* const b{Take(kB)};
    Deadline* const c{Take(kC)};
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    auto handle_a = StartAt(*a, 0ms);
    auto handle_b = StartAt(*b, 0ms);
    auto handle_c = StartAt(*c, 0ms);
    EXPECT_TRUE(EvaluateAt(20ms));

    EXPECT_FALSE(EvaluateAt(21ms));
    EXPECT_FALSE(EvaluateAt(41ms));
}

TEST_F(DeadlineMonitorImplTest, OnlyTheDeadlineThatMissedFailsTheEvaluation)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that one missed deadline among healthy ones makes Evaluate return false.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{10ms, 20ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{10ms, 500ms}});
    CreateMonitor(std::move(deadlines));
    Deadline* const a{Take(kA)};
    Deadline* const b{Take(kB)};
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    auto handle_b = StartAt(*b, 0ms);
    EXPECT_TRUE(EvaluateAt(100ms));

    auto handle_a = StartAt(*a, 100ms);

    EXPECT_TRUE(EvaluateAt(120ms));
    EXPECT_FALSE(EvaluateAt(121ms));
}

TEST_F(DeadlineMonitorImplTest, EvaluateIgnoresTheHealthMonitorStart)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that the result of Evaluate does not depend on the health monitor start time.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);

    now_ = kCreation + 50ms;
    EXPECT_TRUE(monitor_->Evaluate(kCreation));
    EXPECT_TRUE(monitor_->Evaluate(kCreation + 1h));
    now_ = kCreation + 200ms;
    EXPECT_FALSE(monitor_->Evaluate(kCreation));
    EXPECT_FALSE(monitor_->Evaluate(kCreation + 1h));
}

TEST_F(DeadlineMonitorImplTest, StartAfterAnEarlyStopFailsWithDeadlineFailed)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_failed_reuse, comp_req__health_monitor__failure_latch");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a deadline that stopped too early refuses a start and Evaluate stays false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    {
        auto handle = StartAt(*deadline, 10ms);
        StopAt(handle, 20ms);
    }

    now_ = kCreation + 500ms;
    ExpectError(deadline->Start(), ErrorCode::kDeadlineFailed);

    EXPECT_FALSE(EvaluateAt(500ms));
    EXPECT_FALSE(EvaluateAt(900ms));
}

TEST_F(DeadlineMonitorImplTest, StartAfterALateStopFailsWithDeadlineFailed)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_failed_reuse, comp_req__health_monitor__failure_latch");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a deadline that stopped too late refuses a start and Evaluate stays false.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    {
        auto handle = StartAt(*deadline, 10ms);
        StopAt(handle, 500ms);
    }

    ExpectError(deadline->Start(), ErrorCode::kDeadlineFailed);

    EXPECT_FALSE(EvaluateAt(500ms));
    EXPECT_FALSE(EvaluateAt(900ms));
}

TEST_F(DeadlineMonitorImplTest, StartOfAFailedDeadlineLeavesTheOtherDeadlineUsable)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_failed_reuse");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a failure of one deadline does not block the start of another one.");

    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{80ms, 120ms}});
    deadlines.push_back(DeadlineConfig{kB, TimeRange{80ms, 120ms}});
    CreateMonitor(std::move(deadlines));
    Deadline* const a{Take(kA)};
    Deadline* const b{Take(kB)};
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    {
        auto handle = StartAt(*a, 0ms);
        StopAt(handle, 10ms);
    }

    now_ = kCreation + 20ms;
    auto result = b->Start();

    ASSERT_TRUE(result.has_value());
    StopAt(result.value(), 120ms);
}

TEST_F(DeadlineMonitorImplTest, EvaluateFailsWhileTheTooEarlyErrorIsLogged)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__dl_too_early, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that Evaluate returns false while the error about an early stop is logged.");

    CreateMonitor(Single());
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    auto handle = StartAt(*deadline, 10ms);
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    bool inside_log{false};
    bool healthy_during_log{true};
    // The recorder call stands in for a log backend that blocks while the worker evaluates.
    EXPECT_CALL(recorder, StartRecord(::testing::_, score::mw::log::LogLevel::kError))
        .WillRepeatedly(
            [this, &inside_log, &healthy_during_log](const std::string_view, const score::mw::log::LogLevel) {
                if (!inside_log)
                {
                    inside_log = true;
                    healthy_during_log = monitor_->Evaluate(kCreation);
                }
                return score::cpp::optional<score::mw::log::SlotHandle>{};
            });
    const ScopedLogRecorder scoped_recorder{recorder};

    StopAt(handle, 20ms);

    EXPECT_TRUE(inside_log);
    EXPECT_FALSE(healthy_during_log);
}

TEST_F(DeadlineMonitorImplTest, StartAndStopInsideTheRangeNeverFailAConcurrentEvaluate)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that a thread that starts and stops a deadline inside its range does not make Evaluate on "
        "another thread return false.");

    // The range starts at 0 ms, so a stop in the same millisecond as the start is inside the range.
    // The clock stays fixed, because the test must not change it while the other thread reads it.
    std::vector<DeadlineConfig> deadlines;
    deadlines.push_back(DeadlineConfig{kA, TimeRange{0ms, 120ms}});
    CreateMonitor(std::move(deadlines));
    Deadline* const deadline{Take(kA)};
    ASSERT_NE(deadline, nullptr);
    constexpr int kCycles{5000};
    std::atomic<bool> done{false};
    std::atomic<int> failed_evaluations{0};
    std::atomic<int> evaluations{0};
    std::thread evaluator{[&] {
        do
        {
            if (!monitor_->Evaluate(kCreation))
            {
                ++failed_evaluations;
            }
            ++evaluations;
        } while (!done.load());
    }};

    int failed_starts{0};
    for (int i{0}; i < kCycles; ++i)
    {
        auto result = deadline->Start();
        if (!result.has_value())
        {
            ++failed_starts;
            continue;
        }
        result.value().Stop();
    }
    done = true;
    evaluator.join();

    EXPECT_EQ(failed_starts, 0);
    EXPECT_EQ(failed_evaluations.load(), 0);
    EXPECT_GE(evaluations.load(), 1);
}

TEST_F(DeadlineMonitorImplTest, TakenDeadlineWorksAfterTheTakenMonitorIsDestroyed)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that a taken deadline keeps its monitor alive after every other handle of the monitor is "
        "gone.");

    CreateMonitor(Single());
    ASSERT_NE(monitor_, nullptr);
    const std::weak_ptr<Monitor> weak{monitor_};
    Monitor* const raw{monitor_.get()};
    {
        Taken<DeadlineMonitor> taken_monitor{std::move(monitor_)};
        auto result = taken_monitor->GetDeadline(kA);
        ASSERT_TRUE(result.has_value());
        taken_.push_back(std::move(result).value());
    }
    monitor_.reset();
    ASSERT_FALSE(weak.expired());

    auto handle = StartAt(*taken_.back(), 10ms);
    StopAt(handle, 100ms);

    now_ = kCreation + 100ms;
    EXPECT_TRUE(raw->Evaluate(kCreation));
    taken_.clear();
    EXPECT_TRUE(weak.expired());
}

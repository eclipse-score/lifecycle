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

#include "score/health_monitor/src/cpp/internal/worker.h"

#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/health_monitor/thread_parameters.h"
#include "score/mw/log/recorder_mock.h"
#include "score/os/time.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <climits>
#include <cstddef>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#if defined(__QNX__)
#include <sys/neutrino.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;

namespace
{

/// @brief Counts evaluations and runs on the worker thread.
class FakeEvaluator final : public MonitorEvaluator
{
  public:
    bool Evaluate(const std::chrono::steady_clock::time_point) noexcept override
    {
        const int call{calls.fetch_add(1) + 1};
        const bool passed{call < fail_from_call.load()};
        std::this_thread::sleep_for(delay);
        if (inspect)
        {
            inspect();
        }
        return passed;
    }

    std::atomic<int> calls{0};
    std::atomic<int> fail_from_call{1 << 30};
    std::chrono::milliseconds delay{0};
    void (*inspect)(){nullptr};
};

class FakeClient final : public SupervisorClient
{
  public:
    score::Result<void> Connect() noexcept override
    {
        return {};
    }

    void NotifyAlive() noexcept override
    {
        notifications.fetch_add(1);
    }

    std::atomic<int> notifications{0};
};

class WorkerTest : public ::testing::Test
{
  protected:
    std::unique_ptr<Worker> Start(
        const std::chrono::milliseconds supervisor_cycle,
        const std::chrono::milliseconds internal_cycle)
    {
        pointers_ = {&evaluator_};
        return std::make_unique<Worker>(
            score::cpp::span<MonitorEvaluator* const>{pointers_},
            supervisor_cycle,
            client_,
            internal_cycle,
            parameters_,
            score::os::Time::instance());
    }

    FakeEvaluator evaluator_;
    FakeClient client_;
    std::vector<MonitorEvaluator*> pointers_;
    // Outlives the worker, which keeps a reference.
    ThreadParameters parameters_{};
};

}  // namespace

TEST_F(WorkerTest, HealthyEvaluatorKeepsNotifyingAlive)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__alive_notify, comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that a worker with a healthy monitor evaluates it every cycle and keeps notifying alive.");

    const auto worker = Start(5ms, 5ms);

    EXPECT_TRUE(WaitFor([&] {
        return evaluator_.calls.load() >= 5;
    }));
    EXPECT_TRUE(WaitFor([&] {
        return client_.notifications.load() >= 3;
    }));
}

TEST_F(WorkerTest, HealthyCyclesDoNotLogOnTheWorkerThread)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__detect_latency, comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that a worker whose monitors pass logs nothing on its thread, so a blocked log backend cannot "
        "delay an evaluation or an alive notification.");

    const std::thread::id test_thread{std::this_thread::get_id()};
    std::atomic<int> worker_records{0};
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    ON_CALL(recorder, StartRecord(::testing::_, ::testing::_))
        .WillByDefault([test_thread, &worker_records](const std::string_view, const score::mw::log::LogLevel) {
            if (std::this_thread::get_id() != test_thread)
            {
                worker_records.fetch_add(1);
            }

            return score::cpp::optional<score::mw::log::SlotHandle>{};
        });
    // Installed before the worker starts and removed after it is joined, so the worker never sees the switch.
    const ScopedLogRecorder scoped_recorder{recorder};
    auto worker = Start(5ms, 5ms);

    ASSERT_TRUE(WaitFor([&] {
        return evaluator_.calls.load() >= 5;
    }));
    ASSERT_TRUE(WaitFor([&] {
        return client_.notifications.load() >= 3;
    }));
    const int records_while_healthy{worker_records.load()};
    worker.reset();

    EXPECT_EQ(records_while_healthy, 0);
}

TEST_F(WorkerTest, FailingEvaluatorStopsNotificationsAndEvaluations)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__failure_latch, comp_req__health_monitor__alive_withhold");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that after the first failure the worker neither notifies alive nor evaluates the monitors again.");

    evaluator_.fail_from_call = 1;
    const auto worker = Start(1ms, 5ms);

    ASSERT_TRUE(WaitFor([&] {
        return evaluator_.calls.load() >= 1;
    }));
    // A negative property needs an observation window: well over ten cycles.
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(evaluator_.calls.load(), 1);
    EXPECT_EQ(client_.notifications.load(), 0);
}

TEST_F(WorkerTest, FailureAfterHealthyCyclesLatches)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__failure_latch");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a failure after healthy cycles stops further evaluations and notifications.");

    evaluator_.fail_from_call = 4;
    const auto worker = Start(1ms, 5ms);

    ASSERT_TRUE(WaitFor([&] {
        return evaluator_.calls.load() >= 4;
    }));
    // Settle: the failing run ends before the notification count can change again.
    std::this_thread::sleep_for(100ms);
    const int notifications{client_.notifications.load()};
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(evaluator_.calls.load(), 4);
    EXPECT_EQ(client_.notifications.load(), notifications);
}

TEST_F(WorkerTest, EvaluatorSlowerThanTheCycleKeepsTheWorkerRunning)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that an evaluation that overruns the internal cycle neither stops the worker nor corrupts the timing.");

    evaluator_.delay = 25ms;
    const auto worker = Start(5ms, 5ms);

    EXPECT_TRUE(WaitFor([&] {
        return evaluator_.calls.load() >= 4;
    }));
    EXPECT_TRUE(WaitFor([&] {
        return client_.notifications.load() >= 1;
    }));
}

TEST_F(WorkerTest, DestructionDoesNotWaitOutTheCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that destroying a worker with a ten second cycle returns in well under one second.");

    auto worker = Start(10s, 10s);

    const auto begin = std::chrono::steady_clock::now();
    worker.reset();
    const auto elapsed = std::chrono::steady_clock::now() - begin;

    EXPECT_LT(elapsed, 1s);
    EXPECT_EQ(evaluator_.calls.load(), 0);
}

TEST_F(WorkerTest, CycleAtTheClockLimitWaitsForTheStopRequest)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that a worker whose cycle end lies beyond the steady clock range does not evaluate and "
        "still stops at once on destruction.");

    const std::chrono::milliseconds clock_limit{
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max())};
    auto worker = Start(clock_limit, clock_limit);

    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(evaluator_.calls.load(), 0);
    EXPECT_EQ(client_.notifications.load(), 0);

    const auto begin = std::chrono::steady_clock::now();
    worker.reset();
    EXPECT_LT(std::chrono::steady_clock::now() - begin, 1s);
}

namespace
{

/// @brief CPU indices the calling thread may run on.
std::vector<std::size_t> AllowedCpus()
{
    std::vector<std::size_t> cpus;
#if defined(__QNX__)
    unsigned runmask{0U};
    if (::ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &runmask) != -1)
    {
        for (std::size_t cpu{0U}; cpu < sizeof(runmask) * CHAR_BIT; ++cpu)
        {
            if ((runmask & (1U << cpu)) != 0U)
            {
                cpus.push_back(cpu);
            }
        }
    }
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    if (::pthread_getaffinity_np(::pthread_self(), sizeof(set), &set) == 0)
    {
        for (std::size_t cpu{0U}; cpu < CPU_SETSIZE; ++cpu)
        {
            if (CPU_ISSET(cpu, &set) != 0)
            {
                cpus.push_back(cpu);
            }
        }
    }
#endif
    return cpus;
}

std::atomic<std::size_t> g_observed_cpu_count{0U};
std::atomic<std::size_t> g_observed_first_cpu{0U};

void RecordAffinity()
{
    const std::vector<std::size_t> cpus{AllowedCpus()};
    g_observed_first_cpu = cpus.empty() ? 0U : cpus.front();
    g_observed_cpu_count = cpus.size();
}

}  // namespace

TEST_F(WorkerTest, AppliesTheAffinityToTheWorkerThread)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that the worker thread runs only on the CPU that the affinity names, the highest one the "
        "test thread may use.");

    const std::vector<std::size_t> allowed{AllowedCpus()};
    ASSERT_FALSE(allowed.empty());
    const std::size_t cpu{allowed.back()};

    g_observed_cpu_count = 0U;
    g_observed_first_cpu = 0U;
    evaluator_.inspect = &RecordAffinity;
    const std::vector<std::size_t> cpus{cpu};
    parameters_ = ThreadParameters{}.WithAffinity(cpus);
    const auto worker = Start(5ms, 5ms);

    ASSERT_TRUE(WaitFor([&] {
        return g_observed_cpu_count.load() != 0U;
    }));
    EXPECT_EQ(g_observed_cpu_count.load(), 1U);
    EXPECT_EQ(g_observed_first_cpu.load(), cpu);
}

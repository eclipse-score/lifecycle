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
#include <gtest/gtest.h>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "score/mw/lifecycle/report_running.h"
#include "score/mw/log/rust/stdout_logger_init.h"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/health/common.h>
#include <score/mw/health/health_monitor.h>

namespace
{

using namespace score::mw::health;
using namespace std::chrono_literals;

constexpr std::size_t kNumMonitors = 1024U;
constexpr std::size_t kNumThreads = 16U;

/// Monitor tags have the fixed form "hb_monitor_NNNN", because a tag keeps a pointer to a character array
/// and takes its length from the size of the array.
constexpr char kTagPrefix[] = "hb_monitor_";
constexpr std::size_t kTagDigits = 4U;
constexpr std::size_t kTagSize = sizeof(kTagPrefix) - 1U + kTagDigits + 1U;
using TagStorage = char[kTagSize];

/// Every monitor expects a heartbeat 50 ms to 500 ms after the previous one and gets one every 150 ms,
/// which leaves room for scheduling delays while the process is under load.
constexpr std::chrono::milliseconds kHeartbeatPeriod{150ms};
constexpr std::chrono::milliseconds kHeartbeatRangeMin{50ms};
constexpr std::chrono::milliseconds kHeartbeatRangeMax{500ms};

/// Must be shorter than two times kHeartbeatRangeMin.
constexpr std::chrono::milliseconds kInternalProcessingCycle{50ms};
constexpr std::chrono::milliseconds kSupervisorApiCycle{50ms};

/// How long the heartbeats are sent. Afterwards they stop and the health monitor has to detect it.
constexpr std::chrono::milliseconds kLoadDuration{10s};

TagStorage g_tags[kNumMonitors]{};

void make_tag(TagStorage& tag, std::size_t index)
{
    std::memcpy(tag, kTagPrefix, sizeof(kTagPrefix) - 1U);
    for (std::size_t digit = 0U; digit < kTagDigits; ++digit)
    {
        tag[kTagSize - 2U - digit] = static_cast<char>('0' + static_cast<int>(index % 10U));
        index /= 10U;
    }
    tag[kTagSize - 1U] = '\0';
}

}  // namespace

TEST(StressHealthMonitor, ManyHeartbeatMonitors)
{
    // Rust logger required for hmon logs
    score::mw::log::rust::StdoutLoggerBuilder logger_builder;
    logger_builder.Context("APP").LogLevel(score::mw::log::rust::LogLevel::Info).SetAsDefaultLogger();

    for (std::size_t index = 0U; index < kNumMonitors; ++index)
    {
        make_tag(g_tags[index], index);
    }

    // HealthMonitorBuilder can only be moved into a new builder, not assigned, so it is replaced in place.
    std::optional<HealthMonitorBuilder> builder{std::in_place};
    TEST_STEP("Add heartbeat monitors")
    {
        for (std::size_t index = 0U; index < kNumMonitors; ++index)
        {
            builder.emplace(
                std::move(*builder).AddHeartbeatMonitor(
                    MonitorTag(g_tags[index]),
                    heartbeat::HeartbeatMonitorBuilder(TimeRange(kHeartbeatRangeMin, kHeartbeatRangeMax))));
        }
    }

    auto hm_result = std::move(*builder)
                         .WithInternalProcessingCycle(kInternalProcessingCycle)
                         .WithSupervisorApiCycle(kSupervisorApiCycle)
                         .Build();
    ASSERT_TRUE(hm_result.has_value()) << "Failed to build HealthMonitor with " << kNumMonitors << " monitors";
    auto hm = std::move(*hm_result);

    std::vector<heartbeat::HeartbeatMonitor> monitors;
    monitors.reserve(kNumMonitors);
    TEST_STEP("Get heartbeat monitors")
    {
        for (std::size_t index = 0U; index < kNumMonitors; ++index)
        {
            auto monitor_result = hm.GetHeartbeatMonitor(MonitorTag(g_tags[index]));
            ASSERT_TRUE(monitor_result.has_value()) << "Failed to get heartbeat monitor " << index;
            monitors.push_back(std::move(*monitor_result));
        }
    }

    hm.Start();

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    // The health monitor withholds the alive notifications if any monitor misses a heartbeat, which makes the
    // Launch Manager execute the recovery action. The control client test driver verifies that this happens only
    // after the heartbeats stopped.
    TEST_STEP("Send heartbeats from all threads for 10 seconds")
    {
        const auto load_end = std::chrono::steady_clock::now() + kLoadDuration;
        std::vector<std::thread> threads;
        threads.reserve(kNumThreads);
        for (std::size_t thread_index = 0U; thread_index < kNumThreads; ++thread_index)
        {
            threads.emplace_back([&monitors, thread_index, load_end]() {
                while (!TestRunner::g_exit_requested && (std::chrono::steady_clock::now() < load_end))
                {
                    // Sleeping before each round keeps every gap between two heartbeats at least kHeartbeatPeriod.
                    std::this_thread::sleep_for(kHeartbeatPeriod);
                    for (std::size_t index = thread_index; index < monitors.size(); index += kNumThreads)
                    {
                        monitors[index].Heartbeat();
                    }
                }
            });
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
        EXPECT_FALSE(TestRunner::g_exit_requested) << "Process should not be terminated yet";
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    // The missing heartbeats make the Launch Manager kill this process (verified by the control client test driver).
    TestRunner::WaitForTermination();
    return result;
}

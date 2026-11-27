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

#include "score/concurrency/interruptible_wait.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/monotonic_time.h"
#include "score/health_monitor/src/cpp/internal/thread_setup.h"
#include "score/mw/log/log_stream.h"
#include "score/mw/log/logger.h"

#include <score/span.hpp>

#include <optional>

namespace score::health_monitor::details
{

namespace
{

/// @brief Returns `start + cycle`, or `steady_clock::time_point::max()` where the sum overflows.
/// @note `score::concurrency::wait_until()` waits only for the stop request on `time_point::max()`.
std::chrono::steady_clock::time_point CycleEnd(
    const std::chrono::steady_clock::time_point start,
    const std::chrono::steady_clock::duration cycle) noexcept
{
    if (start > (std::chrono::steady_clock::time_point::max() - cycle))
    {
        return std::chrono::steady_clock::time_point::max();
    }

    return start + cycle;
}

}  // namespace

Worker::Worker(
    const score::cpp::span<MonitorEvaluator* const> evaluators,
    const std::chrono::milliseconds supervisor_api_cycle,
    SupervisorClient& client,
    const std::chrono::milliseconds internal_processing_cycle,
    const ThreadParameters& thread_parameters,
    const score::os::Time& time) noexcept
    : logic_{evaluators, supervisor_api_cycle, client, time},
      internal_processing_cycle_{internal_processing_cycle},
      thread_parameters_{thread_parameters},
      time_{time},
      // A stack size hint of 0 keeps the default stack size of the operating system.
      thread_{
          score::cpp::jthread::stack_size_hint{thread_parameters.GetStackSize().value_or(0U)},
          [this](const score::cpp::stop_token& stop_token) noexcept {
              Run(stop_token);
          }}
{
}

void Worker::Run(const score::cpp::stop_token& stop_token) noexcept
{
    ApplyThreadParameters(thread_parameters_);

    const std::chrono::steady_clock::time_point hmon_start{ReadMonotonicTime(time_)};
    std::chrono::steady_clock::time_point cycle_start{hmon_start};
    // The wait returns true on a stop request, also in the middle of a cycle.
    // A cycle that overruns its end starts the next one at once.
    while (!score::concurrency::wait_until(stop_token, CycleEnd(cycle_start, internal_processing_cycle_)))
    {
        cycle_start = ReadMonotonicTime(time_);
        if (!logic_.Run(hmon_start))
        {
            GetLogger().LogInfo() << "Monitoring logic failed, stopping thread.";
            break;
        }
    }

    GetLogger().LogInfo() << "Monitoring thread exiting.";
}

}  // namespace score::health_monitor::details

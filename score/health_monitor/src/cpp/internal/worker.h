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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_WORKER_H
#define SCORE_HEALTH_MONITOR_INTERNAL_WORKER_H

#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/src/cpp/internal/monitoring_logic.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/health_monitor/thread_parameters.h"
#include "score/os/time.h"

#include <score/jthread.hpp>
#include <score/span.hpp>
#include <score/stop_token.hpp>

#include <chrono>

namespace score::health_monitor::details
{

/// @brief Thread that runs the monitoring logic once per internal processing cycle until a monitor fails.
/// @note The thread first applies its thread parameters, then waits one cycle before the first evaluation.
/// @note A cycle that takes longer than the internal processing cycle delays the next one, but does not skip it.
/// @note The thread logs only before it terminates the process, after a failure or a stop request, and when the
/// client logs in `NotifyAlive()`, so a blocked log backend delays no cycle in which every monitor passes.
class Worker final
{
  public:
    /// @brief Starts the thread.
    /// @param evaluators Monitors to evaluate. The view and each monitor must outlive the worker.
    /// @param client Connected client. It must outlive the worker.
    /// @param thread_parameters Accepted by `ValidateThreadParameters()`. They must outlive the worker.
    /// @param time OS interface through which the thread reads `CLOCK_MONOTONIC`. It must outlive the worker.
    /// @pre Both cycles are positive and fit in `steady_clock::duration`, as `HealthMonitorImpl::Create()` checks.
    /// @note A failure to create the thread or to apply its parameters terminates the process.
    Worker(
        const score::cpp::span<MonitorEvaluator* const> evaluators,
        const std::chrono::milliseconds supervisor_api_cycle,
        SupervisorClient& client,
        const std::chrono::milliseconds internal_processing_cycle,
        const ThreadParameters& thread_parameters,
        const score::os::Time& time) noexcept;

    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;
    /// @brief Stops the thread and waits for it, also in the middle of a cycle wait.
    ~Worker() = default;
    Worker& operator=(const Worker&) = delete;
    Worker& operator=(Worker&&) = delete;

  private:
    /// @brief Body of the thread.
    void Run(const score::cpp::stop_token& stop_token) noexcept;

    MonitoringLogic logic_;
    std::chrono::milliseconds internal_processing_cycle_;
    const ThreadParameters& thread_parameters_;
    const score::os::Time& time_;
    /// @brief Last member, so the destructor joins the thread before the members it reads go away.
    score::cpp::jthread thread_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_WORKER_H

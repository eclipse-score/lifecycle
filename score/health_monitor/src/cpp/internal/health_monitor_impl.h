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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_HEALTH_MONITOR_IMPL_H
#define SCORE_HEALTH_MONITOR_INTERNAL_HEALTH_MONITOR_IMPL_H

#include "score/containers/non_relocatable_vector.h"
#include "score/health_monitor/deadline_monitor.h"
#include "score/health_monitor/health_monitor.h"
#include "score/health_monitor/heartbeat_monitor.h"
#include "score/health_monitor/logic_monitor.h"
#include "score/health_monitor/src/cpp/deadline_monitor/deadline_monitor_impl.h"
#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/src/cpp/internal/worker.h"
#include "score/health_monitor/src/cpp/logic_monitor/logic_monitor_impl.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/thread_parameters.h"
#include "score/health_monitor/time_range.h"
#include "score/os/time.h"
#include "score/result/result.h"

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace score::health_monitor::details
{

/// @brief A monitor of the health monitor and whether the application took it.
template <typename Monitor>
struct MonitorEntry
{
    MonitorTag tag;
    std::shared_ptr<Monitor> monitor;
    bool taken{false};
};

/// @brief Health monitor that evaluates its monitors on a worker thread.
class HealthMonitorImpl final : public HealthMonitor
{
  public:
    /// @brief Configuration copied from `HealthMonitorBuilder`.
    struct Config
    {
        /// @brief Deadlines of one deadline monitor.
        struct DeadlineMonitorConfig
        {
            MonitorTag tag;
            std::vector<DeadlineConfig> deadlines;
        };

        /// @brief Tag and time range of one heartbeat monitor.
        struct HeartbeatMonitorConfig
        {
            MonitorTag tag;
            TimeRange range;
        };

        /// @brief State graph of one logic monitor.
        struct LogicMonitorConfig
        {
            MonitorTag tag;
            StateTag initial_state;
            std::vector<StateNode> states;
        };

        std::vector<DeadlineMonitorConfig> deadline_monitors;
        std::vector<HeartbeatMonitorConfig> heartbeat_monitors;
        std::vector<LogicMonitorConfig> logic_monitors;
        std::chrono::milliseconds supervisor_api_cycle{500};
        std::chrono::milliseconds internal_processing_cycle{100};
        ThreadParameters thread_parameters;
    };

    /// @brief Checks the configuration and creates the health monitor with all its monitors.
    /// @param client Client that `Start()` connects. It must not be null.
    /// @param time OS interface through which the monitors and the worker read `CLOCK_MONOTONIC`.
    /// @return The health monitor, or the error of the first failed check, as `HealthMonitorBuilder::Build()` lists.
    /// @pre `time` outlives the health monitor and every monitor taken from it.
    static score::Result<std::unique_ptr<HealthMonitorImpl>>
    Create(Config&& config, std::unique_ptr<SupervisorClient> client, const score::os::Time& time) noexcept;

    HealthMonitorImpl(const HealthMonitorImpl&) = delete;
    HealthMonitorImpl(HealthMonitorImpl&&) = delete;
    /// @brief Stops and joins the worker if it runs.
    ~HealthMonitorImpl() override = default;
    HealthMonitorImpl& operator=(const HealthMonitorImpl&) = delete;
    HealthMonitorImpl& operator=(HealthMonitorImpl&&) = delete;

    score::Result<Taken<DeadlineMonitor>> GetDeadlineMonitor(const MonitorTag& tag) noexcept override;
    score::Result<Taken<HeartbeatMonitor>> GetHeartbeatMonitor(const MonitorTag& tag) noexcept override;
    score::Result<Taken<LogicMonitor>> GetLogicMonitor(const MonitorTag& tag) noexcept override;
    score::Result<void> Start() noexcept override;

  private:
    HealthMonitorImpl(
        score::containers::NonRelocatableVector<MonitorEntry<DeadlineMonitor>>&& deadline_monitors,
        score::containers::NonRelocatableVector<MonitorEntry<HeartbeatMonitor>>&& heartbeat_monitors,
        score::containers::NonRelocatableVector<MonitorEntry<LogicMonitor>>&& logic_monitors,
        score::containers::NonRelocatableVector<MonitorEvaluator*>&& evaluators,
        Config&& config,
        std::unique_ptr<SupervisorClient> client,
        const score::os::Time& time) noexcept;

    score::containers::NonRelocatableVector<MonitorEntry<DeadlineMonitor>> deadline_monitors_;
    score::containers::NonRelocatableVector<MonitorEntry<HeartbeatMonitor>> heartbeat_monitors_;
    score::containers::NonRelocatableVector<MonitorEntry<LogicMonitor>> logic_monitors_;
    /// @brief The same monitors as the entries, as the worker evaluates them.
    score::containers::NonRelocatableVector<MonitorEvaluator*> evaluators_;
    std::chrono::milliseconds supervisor_api_cycle_;
    std::chrono::milliseconds internal_processing_cycle_;
    ThreadParameters thread_parameters_;
    std::unique_ptr<SupervisorClient> client_;
    const score::os::Time& time_;
    /// @brief The worker after a successful `Start()`, also after it stopped at a failure.
    /// Last member, so the worker stops before the members it reads go away.
    std::optional<Worker> worker_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_HEALTH_MONITOR_IMPL_H

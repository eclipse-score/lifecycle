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

#include "score/health_monitor/src/cpp/internal/health_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/heartbeat_monitor/heartbeat_monitor_impl.h"
#include "score/health_monitor/src/cpp/internal/log_string.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/thread_setup.h"

#include <score/assert.hpp>
#include <score/utility.hpp>

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace score::health_monitor::details
{

namespace
{

template <typename Monitor>
score::Result<Taken<Monitor>> TakeMonitor(
    score::containers::NonRelocatableVector<MonitorEntry<Monitor>>& entries,
    const MonitorTag& tag) noexcept
{
    const auto entry = std::find_if(entries.begin(), entries.end(), [&tag](const MonitorEntry<Monitor>& candidate) {
        return candidate.tag == tag;
    });
    if (entry == entries.end())
    {
        return score::MakeUnexpected(ErrorCode::kUnknownTag);
    }

    if (entry->taken)
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyTaken);
    }

    entry->taken = true;
    return Taken<Monitor>{entry->monitor};
}

score::Result<void> CheckCycles(
    const std::chrono::milliseconds supervisor_api_cycle,
    const std::chrono::milliseconds internal_processing_cycle) noexcept
{
    if ((supervisor_api_cycle.count() <= 0) || (internal_processing_cycle.count() <= 0))
    {
        GetLogger().LogError() << "Supervisor API cycle duration (ms)" << supervisor_api_cycle.count()
                               << "and internal processing cycle interval (ms)" << internal_processing_cycle.count()
                               << "must be positive.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    // The worker compares and adds the cycles as `steady_clock::duration`, which a longer cycle overflows.
    const std::chrono::milliseconds kMaxCycle{
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max())};
    if ((supervisor_api_cycle > kMaxCycle) || (internal_processing_cycle > kMaxCycle))
    {
        GetLogger().LogError() << "Supervisor API cycle duration (ms)" << supervisor_api_cycle.count()
                               << "and internal processing cycle interval (ms)" << internal_processing_cycle.count()
                               << "must not exceed (ms)" << kMaxCycle.count();
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    if ((supervisor_api_cycle % internal_processing_cycle).count() != 0)
    {
        GetLogger().LogError() << "Supervisor API cycle duration (ms)" << supervisor_api_cycle.count()
                               << "must be a multiple of internal processing cycle interval (ms)"
                               << internal_processing_cycle.count();
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    return {};
}

score::Result<void> CheckUniqueTags(const HealthMonitorImpl::Config& config) noexcept
{
    score::containers::NonRelocatableVector<MonitorTag> seen{
        config.deadline_monitors.size() + config.heartbeat_monitors.size() + config.logic_monitors.size()};
    const auto add = [&seen](const MonitorTag& tag) noexcept {
        if (std::find(seen.cbegin(), seen.cend(), tag) != seen.cend())
        {
            GetLogger().LogError() << "Monitor tag" << ToLogString(tag) << "is used by more than one monitor.";
            return false;
        }
        score::cpp::ignore = seen.emplace_back(tag);
        return true;
    };

    for (const HealthMonitorImpl::Config::DeadlineMonitorConfig& deadline_monitor : config.deadline_monitors)
    {
        if (!add(deadline_monitor.tag))
        {
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }
    for (const HealthMonitorImpl::Config::HeartbeatMonitorConfig& heartbeat_monitor : config.heartbeat_monitors)
    {
        if (!add(heartbeat_monitor.tag))
        {
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }
    for (const HealthMonitorImpl::Config::LogicMonitorConfig& logic_monitor : config.logic_monitors)
    {
        if (!add(logic_monitor.tag))
        {
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }

    return {};
}

/// @brief Returns the tag of the first monitor of `entries` that is not taken, or no value if all are taken.
template <typename Monitor>
std::optional<MonitorTag> FindUntaken(
    const score::containers::NonRelocatableVector<MonitorEntry<Monitor>>& entries) noexcept
{
    for (const MonitorEntry<Monitor>& entry : entries)
    {
        if (!entry.taken)
        {
            return entry.tag;
        }
    }

    return std::nullopt;
}

}  // namespace

score::Result<std::unique_ptr<HealthMonitorImpl>> HealthMonitorImpl::Create(
    Config&& config,
    std::unique_ptr<SupervisorClient> client,
    const score::os::Time& time) noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(client != nullptr);

    const score::Result<void> cycles{CheckCycles(config.supervisor_api_cycle, config.internal_processing_cycle)};
    if (!cycles.has_value())
    {
        return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(cycles.error());
    }

    if (config.deadline_monitors.empty() && config.heartbeat_monitors.empty() && config.logic_monitors.empty())
    {
        GetLogger().LogError() << "No monitors have been added. HealthMonitor cannot be created.";
        return score::MakeUnexpected(ErrorCode::kNoMonitors);
    }

    const score::Result<void> unique_tags{CheckUniqueTags(config)};
    if (!unique_tags.has_value())
    {
        return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(unique_tags.error());
    }

    const score::Result<void> thread_parameters{ValidateThreadParameters(config.thread_parameters)};
    if (!thread_parameters.has_value())
    {
        return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(thread_parameters.error());
    }

    score::containers::NonRelocatableVector<MonitorEvaluator*> evaluators{
        config.deadline_monitors.size() + config.heartbeat_monitors.size() + config.logic_monitors.size()};

    score::containers::NonRelocatableVector<MonitorEntry<DeadlineMonitor>> deadline_monitors{
        config.deadline_monitors.size()};
    for (const HealthMonitorImpl::Config::DeadlineMonitorConfig& deadline_monitor : config.deadline_monitors)
    {
        auto monitor = DeadlineMonitorImpl::Create(deadline_monitor.tag, deadline_monitor.deadlines, time);
        if (!monitor.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(monitor.error());
        }

        score::cpp::ignore = evaluators.emplace_back(monitor->get());
        score::cpp::ignore = deadline_monitors.emplace_back(
            MonitorEntry<DeadlineMonitor>{deadline_monitor.tag, *std::move(monitor), false});
    }

    score::containers::NonRelocatableVector<MonitorEntry<HeartbeatMonitor>> heartbeat_monitors{
        config.heartbeat_monitors.size()};
    for (const HealthMonitorImpl::Config::HeartbeatMonitorConfig& heartbeat_monitor : config.heartbeat_monitors)
    {
        auto monitor = HeartbeatMonitorImpl::Create(
            heartbeat_monitor.tag, heartbeat_monitor.range, config.internal_processing_cycle, time);
        if (!monitor.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(monitor.error());
        }

        score::cpp::ignore = evaluators.emplace_back(monitor->get());
        score::cpp::ignore = heartbeat_monitors.emplace_back(
            MonitorEntry<HeartbeatMonitor>{heartbeat_monitor.tag, *std::move(monitor), false});
    }

    score::containers::NonRelocatableVector<MonitorEntry<LogicMonitor>> logic_monitors{config.logic_monitors.size()};
    for (HealthMonitorImpl::Config::LogicMonitorConfig& logic_monitor : config.logic_monitors)
    {
        auto monitor =
            LogicMonitorImpl::Create(logic_monitor.tag, logic_monitor.initial_state, std::move(logic_monitor.states));
        if (!monitor.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<HealthMonitorImpl>>(monitor.error());
        }

        score::cpp::ignore = evaluators.emplace_back(monitor->get());
        score::cpp::ignore =
            logic_monitors.emplace_back(MonitorEntry<LogicMonitor>{logic_monitor.tag, *std::move(monitor), false});
    }

    return std::unique_ptr<HealthMonitorImpl>{new HealthMonitorImpl{
        std::move(deadline_monitors),
        std::move(heartbeat_monitors),
        std::move(logic_monitors),
        std::move(evaluators),
        std::move(config),
        std::move(client),
        time}};
}

HealthMonitorImpl::HealthMonitorImpl(
    score::containers::NonRelocatableVector<MonitorEntry<DeadlineMonitor>>&& deadline_monitors,
    score::containers::NonRelocatableVector<MonitorEntry<HeartbeatMonitor>>&& heartbeat_monitors,
    score::containers::NonRelocatableVector<MonitorEntry<LogicMonitor>>&& logic_monitors,
    score::containers::NonRelocatableVector<MonitorEvaluator*>&& evaluators,
    Config&& config,
    std::unique_ptr<SupervisorClient> client,
    const score::os::Time& time) noexcept
    : HealthMonitor{},
      deadline_monitors_{std::move(deadline_monitors)},
      heartbeat_monitors_{std::move(heartbeat_monitors)},
      logic_monitors_{std::move(logic_monitors)},
      evaluators_{std::move(evaluators)},
      supervisor_api_cycle_{config.supervisor_api_cycle},
      internal_processing_cycle_{config.internal_processing_cycle},
      thread_parameters_{std::move(config.thread_parameters)},
      client_{std::move(client)},
      time_{time}
{
}

score::Result<Taken<DeadlineMonitor>> HealthMonitorImpl::GetDeadlineMonitor(const MonitorTag& tag) noexcept
{
    if (worker_.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyStarted);
    }

    return TakeMonitor(deadline_monitors_, tag);
}

score::Result<Taken<HeartbeatMonitor>> HealthMonitorImpl::GetHeartbeatMonitor(const MonitorTag& tag) noexcept
{
    if (worker_.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyStarted);
    }

    return TakeMonitor(heartbeat_monitors_, tag);
}

score::Result<Taken<LogicMonitor>> HealthMonitorImpl::GetLogicMonitor(const MonitorTag& tag) noexcept
{
    if (worker_.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyStarted);
    }

    return TakeMonitor(logic_monitors_, tag);
}

score::Result<void> HealthMonitorImpl::Start() noexcept
{
    if (worker_.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyStarted);
    }

    std::optional<MonitorTag> untaken{FindUntaken(deadline_monitors_)};
    if (!untaken.has_value())
    {
        untaken = FindUntaken(heartbeat_monitors_);
    }
    if (!untaken.has_value())
    {
        untaken = FindUntaken(logic_monitors_);
    }

    if (untaken.has_value())
    {
        GetLogger().LogError() << "All monitors must be taken before starting HealthMonitor but"
                               << ToLogString(*untaken) << "is not taken.";
        return score::MakeUnexpected(ErrorCode::kNotTaken);
    }

    const score::Result<void> connected{client_->Connect()};
    if (!connected.has_value())
    {
        return connected;
    }

    // Logged before the worker exists, so a blocked log backend delays only this call and nothing that is monitored.
    GetLogger().LogInfo() << "Starting monitoring thread.";
    score::cpp::ignore = worker_.emplace(
        score::cpp::span<MonitorEvaluator* const>{evaluators_.data(), evaluators_.size()},
        supervisor_api_cycle_,
        *client_,
        internal_processing_cycle_,
        thread_parameters_,
        time_);
    return {};
}

}  // namespace score::health_monitor::details

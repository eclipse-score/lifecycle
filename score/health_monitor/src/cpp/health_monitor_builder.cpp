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

#include "score/health_monitor/health_monitor_builder.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/health_monitor_impl.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/os/time.h"

#include <score/utility.hpp>

#include <utility>
#include <vector>

namespace score::health_monitor
{

HealthMonitorBuilder HealthMonitorBuilder::AddDeadlineMonitor(
    const MonitorTag& tag,
    DeadlineMonitorBuilder&& monitor) && noexcept
{
    score::cpp::ignore = deadline_monitors_.emplace_back(tag, std::move(monitor));
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::AddHeartbeatMonitor(
    const MonitorTag& tag,
    HeartbeatMonitorBuilder&& monitor) && noexcept
{
    score::cpp::ignore = heartbeat_monitors_.emplace_back(tag, std::move(monitor));
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::AddLogicMonitor(
    const MonitorTag& tag,
    LogicMonitorBuilder&& monitor) && noexcept
{
    score::cpp::ignore = logic_monitors_.emplace_back(tag, std::move(monitor));
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::WithSupervisorApiCycle(const std::chrono::milliseconds cycle) && noexcept
{
    supervisor_api_cycle_ = cycle;
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::WithInternalProcessingCycle(
    const std::chrono::milliseconds cycle) && noexcept
{
    internal_processing_cycle_ = cycle;
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::WithThreadParameters(ThreadParameters&& thread_parameters) && noexcept
{
    thread_parameters_ = std::move(thread_parameters);
    return std::move(*this);
}

score::Result<std::unique_ptr<HealthMonitor>> HealthMonitorBuilder::Build() && noexcept
{
    details::HealthMonitorImpl::Config config{};
    config.deadline_monitors.reserve(deadline_monitors_.size());
    for (const auto& tag_and_monitor : deadline_monitors_)
    {
        const DeadlineMonitorBuilder& monitor{tag_and_monitor.second};
        std::vector<details::DeadlineConfig> deadlines;
        deadlines.reserve(monitor.deadlines_.size());
        for (const DeadlineMonitorBuilder::Entry& deadline : monitor.deadlines_)
        {
            score::cpp::ignore = deadlines.emplace_back(details::DeadlineConfig{deadline.tag, deadline.range});
        }

        score::cpp::ignore = config.deadline_monitors.emplace_back(
            details::HealthMonitorImpl::Config::DeadlineMonitorConfig{tag_and_monitor.first, std::move(deadlines)});
    }

    config.heartbeat_monitors.reserve(heartbeat_monitors_.size());
    for (const auto& tag_and_monitor : heartbeat_monitors_)
    {
        score::cpp::ignore = config.heartbeat_monitors.emplace_back(
            details::HealthMonitorImpl::Config::HeartbeatMonitorConfig{
                tag_and_monitor.first, tag_and_monitor.second.range_});
    }

    config.logic_monitors.reserve(logic_monitors_.size());
    for (auto& tag_and_monitor : logic_monitors_)
    {
        LogicMonitorBuilder& monitor{tag_and_monitor.second};
        std::vector<details::StateNode> states;
        states.reserve(monitor.states_.size());
        for (LogicMonitorBuilder::Node& node : monitor.states_)
        {
            score::cpp::ignore = states.emplace_back(details::StateNode{node.state, std::move(node.allowed_states)});
        }

        score::cpp::ignore = config.logic_monitors.emplace_back(
            details::HealthMonitorImpl::Config::LogicMonitorConfig{
                tag_and_monitor.first, monitor.initial_state_, std::move(states)});
    }

    config.supervisor_api_cycle = supervisor_api_cycle_;
    config.internal_processing_cycle = internal_processing_cycle_;
    config.thread_parameters = std::move(thread_parameters_);

    auto health_monitor =
        details::HealthMonitorImpl::Create(std::move(config), SupervisorClient::Create(), score::os::Time::instance());
    if (!health_monitor.has_value())
    {
        return score::MakeUnexpected<std::unique_ptr<HealthMonitor>>(health_monitor.error());
    }

    return std::unique_ptr<HealthMonitor>{*std::move(health_monitor)};
}

}  // namespace score::health_monitor

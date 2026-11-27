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
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/os/time.h"

#include <score/utility.hpp>

#include <utility>

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
    // The component has no deadline or logic monitor implementation.
    if ((!deadline_monitors_.empty()) || (!logic_monitors_.empty()))
    {
        details::GetLogger().LogError() << "Deadline and logic monitors are not supported.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    details::HealthMonitorImpl::Config config{};
    config.heartbeat_monitors.reserve(heartbeat_monitors_.size());
    for (const auto& tag_and_monitor : heartbeat_monitors_)
    {
        score::cpp::ignore = config.heartbeat_monitors.emplace_back(
            details::HealthMonitorImpl::Config::HeartbeatMonitorConfig{
                tag_and_monitor.first, tag_and_monitor.second.range_});
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

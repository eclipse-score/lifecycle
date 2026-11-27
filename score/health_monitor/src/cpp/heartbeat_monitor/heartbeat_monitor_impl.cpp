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

#include "score/health_monitor/src/cpp/heartbeat_monitor/heartbeat_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/monotonic_time.h"

#include <score/assert.hpp>
#include <score/utility.hpp>

#include <optional>

namespace score::health_monitor::details
{

score::Result<std::shared_ptr<HeartbeatMonitorImpl>> HeartbeatMonitorImpl::Create(
    const MonitorTag& tag,
    const TimeRange& range,
    const std::chrono::milliseconds internal_processing_cycle,
    const score::os::Time& time) noexcept
{
    // Same as 2 * min <= cycle, without a multiplication that overflows for a large minimum.
    if (range.Min() <= internal_processing_cycle / 2)
    {
        GetLogger().LogError() << "Heartbeat monitor" << ToLogString(tag) << "Internal processing cycle duration (ms)"
                               << internal_processing_cycle.count()
                               << "must be shorter than two shortest allowed ranges (ms)" << range.Min().count();
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    return std::shared_ptr<HeartbeatMonitorImpl>{new HeartbeatMonitorImpl{tag, range, time}};
}

HeartbeatMonitorImpl::HeartbeatMonitorImpl(
    const MonitorTag& tag,
    const TimeRange& range,
    const score::os::Time& time) noexcept
    : HeartbeatMonitor{},
      MonitorEvaluator{},
      tag_{tag},
      range_min_ms_{static_cast<std::uint64_t>(range.Min().count())},
      range_max_ms_{static_cast<std::uint64_t>(range.Max().count())},
      time_{time},
      monitor_start_{ReadMonotonicTime(time_)}
{
}

void HeartbeatMonitorImpl::Heartbeat() noexcept
{
    const std::uint64_t monitor_now{ElapsedMs(monitor_start_, ReadMonotonicTime(time_))};
    score::cpp::ignore = state_.Update([monitor_now](HeartbeatStateSnapshot current) {
        current.SetHeartbeatTimestamp(monitor_now);
        current.IncrementCounter();
        return std::optional<HeartbeatStateSnapshot>{current};
    });
}

bool HeartbeatMonitorImpl::Evaluate(const std::chrono::steady_clock::time_point hmon_start) noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(hmon_start >= monitor_start_);

    // Get the HMON start and the current time, both in ms since monitor creation.
    const std::uint64_t offset{ElapsedMs(monitor_start_, hmon_start)};
    const std::uint64_t monitor_now{ElapsedMs(monitor_start_, ReadMonotonicTime(time_))};

    // Load and reset the current monitor state.
    const HeartbeatStateSnapshot snapshot{state_.Reset()};

    // `cycle_start_ms_` stays 0 until the first heartbeat in range, so the first cycle starts at the HMON start.
    // A heartbeat sent before the HMON start is therefore before the first range and too early.
    const std::uint64_t cycle_start{(cycle_start_ms_ > 0U) ? cycle_start_ms_ : offset};

    // Get the allowed time range as absolute values.
    const std::uint64_t range_min{cycle_start + range_min_ms_};
    const std::uint64_t range_max{cycle_start + range_max_ms_};

    // Check the current counter state.
    const std::uint8_t counter{snapshot.Counter()};
    // Disallow multiple heartbeats in the same heartbeat cycle.
    if (counter > 1U)
    {
        GetLogger().LogWarn() << "Heartbeat monitor" << ToLogString(tag_) << "Multiple heartbeats detected";
        return false;
    }

    // Handle no heartbeats.
    if (counter == 0U)
    {
        // No heartbeat after the time range is an error.
        // Otherwise it is accepted, but the evaluation stops here.
        if (monitor_now > range_max)
        {
            GetLogger().LogWarn() << "Heartbeat monitor" << ToLogString(tag_)
                                  << "No heartbeat detected, observed after range:" << (monitor_now - range_max);
            return false;
        }

        return true;
    }

    // Check the current heartbeat against the allowed range.
    const std::uint64_t heartbeat{snapshot.HeartbeatTimestamp()};
    // Heartbeat before the allowed range.
    if (heartbeat < range_min)
    {
        GetLogger().LogWarn() << "Heartbeat monitor" << ToLogString(tag_)
                              << "Heartbeat occurred too early, offset to range:" << (range_min - heartbeat);
        return false;
    }
    // Heartbeat after the allowed range.
    if (heartbeat > range_max)
    {
        GetLogger().LogWarn() << "Heartbeat monitor" << ToLogString(tag_)
                              << "Heartbeat occurred too late, offset to range:" << (heartbeat - range_max);
        return false;
    }

    // The heartbeat is in the allowed range and starts a new cycle.
    cycle_start_ms_ = heartbeat;
    return true;
}

}  // namespace score::health_monitor::details

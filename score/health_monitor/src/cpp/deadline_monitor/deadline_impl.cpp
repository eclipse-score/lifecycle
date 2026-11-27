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

#include "score/health_monitor/src/cpp/deadline_monitor/deadline_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/monotonic_time.h"

#include <score/assert.hpp>
#include <limits>
#include <optional>

namespace score::health_monitor::details
{

DeadlineImpl::DeadlineImpl(
    const MonitorTag& monitor_tag,
    const DeadlineTag& tag,
    const TimeRange& range,
    DeadlineState& state,
    const score::os::Time& time,
    const std::chrono::steady_clock::time_point monitor_start) noexcept
    : Deadline{},
      monitor_tag_{monitor_tag},
      tag_{tag},
      range_min_ms_{static_cast<std::uint32_t>(range.Min().count())},
      range_max_ms_{static_cast<std::uint32_t>(range.Max().count())},
      state_{state},
      time_{time},
      monitor_start_{monitor_start}
{
}

score::Result<void> DeadlineImpl::DoStart() noexcept
{
    const std::uint64_t now{ElapsedMs(monitor_start_, ReadMonotonicTime(time_))};
    const std::uint64_t end{now + range_max_ms_};
    // The expected end has 32 bits, so a later end is a time calculation overflow and the process fails silent.
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        end <= std::numeric_limits<std::uint32_t>::max(), "Deadline expected end does not fit 32 bits");
    const std::uint32_t max_time{static_cast<std::uint32_t>(end)};

    const auto started = state_.Update([max_time](const DeadlineStateSnapshot current) {
        // A deadline that failed earlier keeps its state, so the worker still reports it.
        if (current.IsRunning() || current.IsTooEarly())
        {
            return std::optional<DeadlineStateSnapshot>{};
        }

        DeadlineStateSnapshot next{};
        next.SetTimestampMs(max_time);
        next.SetRunning();
        return std::optional<DeadlineStateSnapshot>{next};
    });
    if (!started.has_value())
    {
        GetLogger().LogWarn() << "Deadline monitor" << ToLogString(monitor_tag_) << "Trying to start deadline"
                              << ToLogString(tag_) << "that already failed";
        return score::MakeUnexpected(ErrorCode::kDeadlineFailed);
    }

    return {};
}

void DeadlineImpl::DoStop() noexcept
{
    const std::uint64_t now{ElapsedMs(monitor_start_, ReadMonotonicTime(time_))};

    const auto stopped = state_.Update([this, now](DeadlineStateSnapshot current) {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(current.IsRunning(), "Deadline is not running at stop");

        // A late stop leaves the state running, so the worker reports the missed deadline.
        if (current.TimestampMs() < now)
        {
            return std::optional<DeadlineStateSnapshot>{};
        }

        // An early stop sets the too-early flag, so the worker reports the deadline.
        if (now < EarliestStopMs(current.TimestampMs()))
        {
            current.SetTooEarly();
            return std::optional<DeadlineStateSnapshot>{current};
        }

        return std::optional<DeadlineStateSnapshot>{DeadlineStateSnapshot{}};
    });

    // The returned snapshot is the state that the update checked, so it gives the offset of a failed stop.
    if (!stopped.has_value())
    {
        GetLogger().LogError() << "Deadline monitor" << ToLogString(monitor_tag_) << "Deadline" << ToLogString(tag_)
                               << "stopped too late by" << (now - stopped.error().TimestampMs()) << "ms";
        return;
    }

    const std::uint32_t earliest_time{EarliestStopMs(stopped.value().TimestampMs())};
    if (now < earliest_time)
    {
        GetLogger().LogError() << "Deadline monitor" << ToLogString(monitor_tag_) << "Deadline" << ToLogString(tag_)
                               << "stopped too early by" << (earliest_time - now) << "ms";
    }
}

std::uint32_t DeadlineImpl::EarliestStopMs(const std::uint32_t expected) const noexcept
{
    const std::uint32_t start_time{expected - range_max_ms_};
    return start_time + range_min_ms_;
}

}  // namespace score::health_monitor::details

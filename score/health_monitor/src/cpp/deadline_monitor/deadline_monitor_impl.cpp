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

#include "score/health_monitor/src/cpp/deadline_monitor/deadline_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/log_string.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/monotonic_time.h"
#include "score/mw/log/log_stream.h"
#include "score/mw/log/log_types.h"
#include "score/mw/log/logger.h"

#include <score/assert.hpp>
#include <score/utility.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace score::health_monitor::details
{

namespace
{

/// @brief Returns the tag of the first deadline whose tag another deadline also has, or no value if each is unique.
std::optional<DeadlineTag> FindRepeatedTag(const std::vector<DeadlineConfig>& deadlines) noexcept
{
    for (const DeadlineConfig& deadline : deadlines)
    {
        const auto count =
            std::count_if(deadlines.cbegin(), deadlines.cend(), [&deadline](const DeadlineConfig& other) noexcept {
                return other.tag == deadline.tag;
            });
        if (count > 1)
        {
            return deadline.tag;
        }
    }

    return std::nullopt;
}

}  // namespace

score::Result<std::shared_ptr<DeadlineMonitorImpl>> DeadlineMonitorImpl::Create(
    const MonitorTag& tag,
    const std::vector<DeadlineConfig>& deadlines,
    const score::os::Time& time) noexcept
{
    if (deadlines.empty())
    {
        GetLogger().LogError() << "Deadline monitor" << ToLogString(tag)
                               << "No deadlines have been added. DeadlineMonitor cannot be created.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    // Each deadline is taken by its tag, so a repeated tag would leave a deadline that no one can take.
    const std::optional<DeadlineTag> repeated_tag{FindRepeatedTag(deadlines)};
    if (repeated_tag.has_value())
    {
        GetLogger().LogError() << "Deadline monitor" << ToLogString(tag) << "Deadline" << ToLogString(*repeated_tag)
                               << "is defined more than once.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    // The expected end of a deadline is its start plus the range maximum, and it has 32 bits.
    const std::uint32_t largest_range_ms{std::numeric_limits<std::uint32_t>::max()};
    for (const DeadlineConfig& deadline : deadlines)
    {
        if (static_cast<std::uint64_t>(deadline.range.Max().count()) > largest_range_ms)
        {
            GetLogger().LogError() << "Deadline monitor" << ToLogString(tag) << "Deadline" << ToLogString(deadline.tag)
                                   << "range maximum (ms)" << deadline.range.Max().count() << "must be at most (ms)"
                                   << largest_range_ms;
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }

    return std::shared_ptr<DeadlineMonitorImpl>{new DeadlineMonitorImpl{tag, deadlines, time}};
}

DeadlineMonitorImpl::DeadlineTemplate::DeadlineTemplate(
    const MonitorTag& monitor_tag,
    const DeadlineConfig& config,
    DeadlineState& state,
    const score::os::Time& time,
    const std::chrono::steady_clock::time_point monitor_start) noexcept
    : ReleaseHook{},
      tag{config.tag},
      in_use{false},
      deadline{monitor_tag, config.tag, config.range, state, time, monitor_start}
{
}

bool DeadlineMonitorImpl::DeadlineTemplate::TryAcquire() noexcept
{
    // Acquire pairs with the release in OnRelease(), so the next holder sees every write of the previous one.
    bool expected{false};
    return in_use.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed);
}

void DeadlineMonitorImpl::DeadlineTemplate::OnRelease() noexcept
{
    in_use.store(false, std::memory_order_release);
}

DeadlineMonitorImpl::ActiveDeadline::ActiveDeadline(const DeadlineTag& deadline_tag) noexcept
    : tag{deadline_tag}, state{}
{
}

DeadlineMonitorImpl::DeadlineMonitorImpl(
    const MonitorTag& tag,
    const std::vector<DeadlineConfig>& deadlines,
    const score::os::Time& time) noexcept
    : DeadlineMonitor{},
      MonitorEvaluator{},
      std::enable_shared_from_this<DeadlineMonitorImpl>{},
      tag_{tag},
      time_{time},
      monitor_start_{ReadMonotonicTime(time_)},
      templates_{deadlines.size()},
      active_deadlines_{deadlines.size()}
{
    for (const DeadlineConfig& deadline : deadlines)
    {
        ActiveDeadline& active{active_deadlines_.emplace_back(deadline.tag)};
        score::cpp::ignore = templates_.emplace_back(tag_, deadline, active.state, time_, monitor_start_);
    }
}

score::Result<Taken<Deadline>> DeadlineMonitorImpl::GetDeadline(const DeadlineTag& tag) noexcept
{
    const auto deadline_template =
        std::find_if(templates_.begin(), templates_.end(), [&tag](const DeadlineTemplate& candidate) noexcept {
            return candidate.tag == tag;
        });
    if (deadline_template == templates_.end())
    {
        return score::MakeUnexpected(ErrorCode::kUnknownTag);
    }

    if (!deadline_template->TryAcquire())
    {
        return score::MakeUnexpected(ErrorCode::kAlreadyTaken);
    }

    // `Create()` returns every monitor in a `shared_ptr`, so the lock returns no `nullptr`.
    std::shared_ptr<DeadlineMonitorImpl> self{weak_from_this().lock()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD(self != nullptr);

    // The aliasing constructor shares the control block of this monitor, which owns the deadline and its template.
    std::shared_ptr<Deadline> deadline{std::move(self), &deadline_template->deadline};
    return Taken<Deadline>{std::move(deadline), *deadline_template};
}

bool DeadlineMonitorImpl::Evaluate(const std::chrono::steady_clock::time_point) noexcept
{
    bool healthy{true};
    // Reading the clock before any state keeps a stop in time between the two reads from being reported as missed.
    const std::uint64_t now{ElapsedMs(monitor_start_, ReadMonotonicTime(time_))};
    for (const ActiveDeadline& active : active_deadlines_)
    {
        const DeadlineStateSnapshot snapshot{active.state.Snapshot()};
        if (snapshot.IsTooEarly())
        {
            GetLogger().LogWarn() << "Deadline monitor" << ToLogString(tag_) << "Deadline" << ToLogString(active.tag)
                                  << "finished too early!";
            healthy = false;
        }
        else if (snapshot.IsRunning())
        {
            SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
                snapshot.IsStopped(), "Running deadline snapshot lost its stopped flag");

            const std::uint32_t expected{snapshot.TimestampMs()};
            if (now > expected)
            {
                GetLogger().LogWarn() << "Deadline monitor" << ToLogString(tag_) << "Deadline"
                                      << ToLogString(active.tag) << "missed! Expected:" << expected << ", now:" << now;
                healthy = false;
            }
        }
        else
        {
            // A stopped deadline passed its last check.
        }
    }

    return healthy;
}

}  // namespace score::health_monitor::details

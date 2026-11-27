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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_BUILDER_H
#define SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_BUILDER_H

#include "score/health_monitor/tag.h"
#include "score/health_monitor/time_range.h"

#include <vector>

namespace score::health_monitor
{

class HealthMonitorBuilder;

/// @brief Configuration of one deadline monitor, given to `HealthMonitorBuilder::AddDeadlineMonitor()`.
class DeadlineMonitorBuilder final
{
  public:
    /// @brief Adds a deadline that must stop within `range` after it starts.
    /// @note A second deadline with the same tag replaces the first.
    DeadlineMonitorBuilder AddDeadline(const DeadlineTag& tag, const TimeRange& range) && noexcept;

  private:
    friend class HealthMonitorBuilder;

    /// @brief One deadline and its range.
    struct Entry
    {
        DeadlineTag tag;
        TimeRange range;
    };

    /// @brief Deadlines of the monitor, one per tag.
    std::vector<Entry> deadlines_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_BUILDER_H

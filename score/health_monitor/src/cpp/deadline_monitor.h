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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_H
#define SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_H

#include "score/health_monitor/deadline.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/taken.h"
#include "score/result/result.h"

namespace score::health_monitor
{

/// @brief Owns the deadlines added with `DeadlineMonitorBuilder::AddDeadline()`.
/// @note Threads can call `GetDeadline()` concurrently. Of the calls for one tag, at most one gets the deadline.
class DeadlineMonitor
{
  public:
    DeadlineMonitor(const DeadlineMonitor&) = delete;
    DeadlineMonitor(DeadlineMonitor&&) = delete;
    virtual ~DeadlineMonitor() = default;
    DeadlineMonitor& operator=(const DeadlineMonitor&) = delete;
    DeadlineMonitor& operator=(DeadlineMonitor&&) = delete;

    /// @brief Returns the deadline with this tag, before or after `HealthMonitor::Start()`.
    /// @return The deadline, valid also after the health monitor is destroyed, or one of these errors:
    /// `ErrorCode::kUnknownTag` if the monitor has no deadline with this tag.
    /// `ErrorCode::kAlreadyTaken` if a handle of this deadline exists.
    /// @note Destroying the handle frees the deadline for the next call. A failed deadline stays failed.
    /// A call concurrent with the destruction of the handle gets the deadline or `ErrorCode::kAlreadyTaken`.
    virtual score::Result<Taken<Deadline>> GetDeadline(const DeadlineTag& tag) noexcept = 0;

  protected:
    DeadlineMonitor() = default;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_H

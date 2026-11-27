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

#ifndef SCORE_HEALTH_MONITOR_LOGIC_MONITOR_H
#define SCORE_HEALTH_MONITOR_LOGIC_MONITOR_H

#include "score/health_monitor/tag.h"
#include "score/result/result.h"

namespace score::health_monitor
{

/// @brief Checks each state transition against the graph given to `LogicMonitorBuilder`.
/// @note The class is not thread-safe.
/// @note An invalid transition puts the monitor in a failed state that it never leaves.
class LogicMonitor
{
  public:
    LogicMonitor(const LogicMonitor&) = delete;
    LogicMonitor(LogicMonitor&&) = delete;
    virtual ~LogicMonitor() = default;
    LogicMonitor& operator=(const LogicMonitor&) = delete;
    LogicMonitor& operator=(LogicMonitor&&) = delete;

    /// @brief Moves to `state`.
    /// @return `state`, or one of these errors:
    /// `ErrorCode::kInvalidTransition` if the current state does not list `state` or the graph has no `state`.
    /// This error puts the monitor in the failed state.
    /// `ErrorCode::kInvalidState` if the monitor was already failed, without a change to the state.
    virtual score::Result<StateTag> Transition(const StateTag& state) noexcept = 0;

    /// @brief Returns the current state.
    /// @return The current state, or `ErrorCode::kInvalidState` if the monitor is failed.
    virtual score::Result<StateTag> State() const noexcept = 0;

  protected:
    LogicMonitor() = default;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_LOGIC_MONITOR_H

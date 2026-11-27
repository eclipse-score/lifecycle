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

#ifndef SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_MONITOR_IMPL_H
#define SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_MONITOR_IMPL_H

#include "score/containers/non_relocatable_vector.h"
#include "score/health_monitor/logic_monitor.h"
#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/src/cpp/logic_monitor/logic_state.h"
#include "score/health_monitor/tag.h"
#include "score/result/result.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

namespace score::health_monitor::details
{

/// @brief One state of a logic monitor and the states it can move to.
struct StateNode
{
    StateTag state;
    std::vector<StateTag> allowed_states;
};

/// @brief Logic monitor that checks each transition against its state graph.
/// @note The first invalid transition latches the monitor: every later `Transition()` and `State()` fails,
/// and every `Evaluate()` returns `false`.
class LogicMonitorImpl final : public LogicMonitor, public MonitorEvaluator
{
  public:
    LogicMonitorImpl(const LogicMonitorImpl&) = delete;
    LogicMonitorImpl(LogicMonitorImpl&&) = delete;
    ~LogicMonitorImpl() override = default;
    LogicMonitorImpl& operator=(const LogicMonitorImpl&) = delete;
    LogicMonitorImpl& operator=(LogicMonitorImpl&&) = delete;

    /// @brief Creates a logic monitor in `initial_state`.
    /// @return The monitor, or `ErrorCode::kInvalidArgument` if `states` is empty, two nodes have the same state,
    /// a node lists a state that no node has, or no node has `initial_state`.
    static score::Result<std::shared_ptr<LogicMonitorImpl>>
    Create(const MonitorTag& tag, const StateTag& initial_state, std::vector<StateNode>&& states) noexcept;

    /// @pre Only one thread calls it. The worker thread can run `Evaluate()` at the same time.
    /// @note An invalid transition fails `Evaluate()` before its warning is logged.
    score::Result<StateTag> Transition(const StateTag& state) noexcept override;

    score::Result<StateTag> State() const noexcept override;

    /// @brief Logs the latched failure with the monitor tag, if the monitor failed.
    /// @param hmon_start Not used: a logic failure does not depend on time.
    /// @return `false` if the monitor failed.
    /// @pre Only the worker thread calls it.
    bool Evaluate(const std::chrono::steady_clock::time_point hmon_start) noexcept override;

  private:
    LogicMonitorImpl(
        const MonitorTag& tag,
        const std::size_t initial_state_index,
        score::containers::NonRelocatableVector<StateNode>&& states) noexcept;

    /// @brief Tag logged with each failure.
    MonitorTag tag_;
    /// @brief State graph. A state index is a position in it.
    score::containers::NonRelocatableVector<StateNode> states_;
    /// @brief Current state index and monitor status.
    LogicState current_state_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_MONITOR_IMPL_H

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

#ifndef SCORE_HEALTH_MONITOR_LOGIC_MONITOR_BUILDER_H
#define SCORE_HEALTH_MONITOR_LOGIC_MONITOR_BUILDER_H

#include "score/health_monitor/tag.h"

#include <score/span.hpp>

#include <vector>

namespace score::health_monitor
{

class HealthMonitorBuilder;

/// @brief State graph of one logic monitor, given to `HealthMonitorBuilder::AddLogicMonitor()`.
class LogicMonitorBuilder final
{
  public:
    /// @brief Creates a state graph that starts in `initial_state`.
    /// @note `HealthMonitorBuilder::Build()` rejects an initial state that no `AddState()` call adds.
    explicit LogicMonitorBuilder(const StateTag& initial_state) noexcept : initial_state_{initial_state}
    {
    }

    /// @brief Adds `state` and the states it can move to.
    /// @param allowed_states Targets of the transitions from `state`. The builder keeps a copy.
    /// A state can move to itself only if this list contains it.
    /// @note A second call with the same `state` replaces the first.
    /// `HealthMonitorBuilder::Build()` rejects a target that no `AddState()` call adds.
    LogicMonitorBuilder AddState(
        const StateTag& state,
        const score::cpp::span<const StateTag> allowed_states) && noexcept;

  private:
    friend class HealthMonitorBuilder;

    /// @brief One state and its transition targets.
    struct Node
    {
        StateTag state;
        std::vector<StateTag> allowed_states;
    };

    /// @brief State before the first transition.
    StateTag initial_state_;
    /// @brief States of the graph, one per tag.
    std::vector<Node> states_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_LOGIC_MONITOR_BUILDER_H

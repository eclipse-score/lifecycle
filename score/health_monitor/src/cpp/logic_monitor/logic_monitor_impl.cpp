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

#include "score/health_monitor/src/cpp/logic_monitor/logic_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/logger.h"

#include <score/utility.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

namespace score::health_monitor::details
{

namespace
{

std::string_view ToString(const LogicEvaluationError error) noexcept
{
    switch (error)
    {
        case LogicEvaluationError::kInvalidState:
            return "InvalidState";
        case LogicEvaluationError::kInvalidTransition:
            return "InvalidTransition";
        case LogicEvaluationError::kUnmappedError:
            return "UnmappedError";
        default:
            return "Unknown";
    }
}

/// @brief Returns the position of the node with `state`, or no value if no node has it.
std::optional<std::size_t> FindIndexByTag(
    const score::containers::NonRelocatableVector<StateNode>& states,
    const StateTag& state) noexcept
{
    const auto node = std::find_if(states.cbegin(), states.cend(), [&state](const StateNode& candidate) noexcept {
        return candidate.state == state;
    });
    if (node == states.cend())
    {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(states.cbegin(), node));
}

/// @brief Returns the state of the first node whose state another node also has, or no value if each is unique.
std::optional<StateTag> FindRepeatedState(const score::containers::NonRelocatableVector<StateNode>& states) noexcept
{
    for (const StateNode& node : states)
    {
        const auto count = std::count_if(states.cbegin(), states.cend(), [&node](const StateNode& other) noexcept {
            return other.state == node.state;
        });
        if (count > 1)
        {
            return node.state;
        }
    }

    return std::nullopt;
}

bool Contains(const std::vector<StateTag>& states, const StateTag& state) noexcept
{
    return std::find(states.cbegin(), states.cend(), state) != states.cend();
}

}  // namespace

score::Result<std::shared_ptr<LogicMonitorImpl>>
LogicMonitorImpl::Create(const MonitorTag& tag, const StateTag& initial_state, std::vector<StateNode>&& states) noexcept
{
    // Check number of states.
    if (states.empty())
    {
        GetLogger().LogError() << "Logic monitor" << ToLogString(tag)
                               << "No states have been added. LogicMonitor cannot be created.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    score::containers::NonRelocatableVector<StateNode> nodes{states.size()};
    for (StateNode& node : states)
    {
        score::cpp::ignore = nodes.emplace_back(std::move(node));
    }

    // Check each state is defined once, so a state index names one node.
    const std::optional<StateTag> repeated_state{FindRepeatedState(nodes)};
    if (repeated_state.has_value())
    {
        GetLogger().LogError() << "Logic monitor" << ToLogString(tag) << "State" << ToLogString(*repeated_state)
                               << "is defined more than once.";
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    // Check transitions are between defined states.
    for (const StateNode& node : nodes)
    {
        for (const StateTag& allowed_state : node.allowed_states)
        {
            if (!FindIndexByTag(nodes, allowed_state).has_value())
            {
                GetLogger().LogError() << "Logic monitor" << ToLogString(tag)
                                       << "Undefined target state. Origin:" << ToLogString(node.state)
                                       << ", target:" << ToLogString(allowed_state);
                return score::MakeUnexpected(ErrorCode::kInvalidArgument);
            }
        }
    }

    // Check initial state is defined, determine initial state index.
    const std::optional<std::size_t> initial_state_index{FindIndexByTag(nodes, initial_state)};
    if (!initial_state_index.has_value())
    {
        GetLogger().LogError() << "Logic monitor" << ToLogString(tag)
                               << "Undefined requested initial state:" << ToLogString(initial_state);
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    return std::shared_ptr<LogicMonitorImpl>{new LogicMonitorImpl{tag, *initial_state_index, std::move(nodes)}};
}

LogicMonitorImpl::LogicMonitorImpl(
    const MonitorTag& tag,
    const std::size_t initial_state_index,
    score::containers::NonRelocatableVector<StateNode>&& states) noexcept
    : LogicMonitor{}, MonitorEvaluator{}, tag_{tag}, states_{std::move(states)}, current_state_{initial_state_index}
{
}

score::Result<StateTag> LogicMonitorImpl::Transition(const StateTag& state) noexcept
{
    // Load current monitor state.
    LogicStateSnapshot snapshot{current_state_.Snapshot()};

    // Disallow operation in erroneous state.
    if (snapshot.MonitorStatus().has_value())
    {
        GetLogger().LogWarn() << "Logic monitor" << ToLogString(tag_)
                              << "Current logic monitor state cannot be determined";
        return score::MakeUnexpected(ErrorCode::kInvalidState);
    }

    // Get name and allowed targets of current state.
    const std::size_t current_state_index{snapshot.CurrentStateIndex()};
    if (current_state_index >= states_.size())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidState);
    }
    const StateNode& current_state_node{states_[current_state_index]};

    // Check transition to a target state is valid.
    // A state that the graph does not have is in no list, so it fails here too.
    if (!Contains(current_state_node.allowed_states, state))
    {
        // Publish the failure before logging it, so the worker does not wait for the log backend to see it.
        snapshot.SetMonitorStatus(LogicEvaluationError::kInvalidTransition);
        score::cpp::ignore = current_state_.Swap(snapshot);
        GetLogger().LogWarn() << "Logic monitor" << ToLogString(tag_)
                              << "Requested state transition is invalid:" << ToLogString(current_state_node.state)
                              << "->" << ToLogString(state);
        return score::MakeUnexpected(ErrorCode::kInvalidTransition);
    }

    // Find index of target state, then change current state.
    const std::optional<std::size_t> target_state_index{FindIndexByTag(states_, state)};
    if (!target_state_index.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidState);
    }
    snapshot.SetCurrentStateIndex(*target_state_index);
    score::cpp::ignore = current_state_.Swap(snapshot);

    return state;
}

score::Result<StateTag> LogicMonitorImpl::State() const noexcept
{
    // Load current monitor state.
    const LogicStateSnapshot snapshot{current_state_.Snapshot()};

    // Disallow operation in erroneous state.
    if (snapshot.MonitorStatus().has_value())
    {
        GetLogger().LogWarn() << "Logic monitor" << ToLogString(tag_)
                              << "Current logic monitor state cannot be determined";
        return score::MakeUnexpected(ErrorCode::kInvalidState);
    }

    // Find current state.
    const std::size_t current_state_index{snapshot.CurrentStateIndex()};
    if (current_state_index >= states_.size())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidState);
    }

    return states_[current_state_index].state;
}

bool LogicMonitorImpl::Evaluate(const std::chrono::steady_clock::time_point) noexcept
{
    const std::optional<LogicEvaluationError> status{current_state_.Snapshot().MonitorStatus()};
    if (status.has_value())
    {
        GetLogger().LogWarn() << "Logic monitor" << ToLogString(tag_)
                              << "Logic monitor error observed:" << ToString(*status);
        return false;
    }

    return true;
}

}  // namespace score::health_monitor::details

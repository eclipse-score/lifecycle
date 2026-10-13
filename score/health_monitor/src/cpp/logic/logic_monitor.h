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
#ifndef SCORE_HM_LOGIC_LOGIC_MONITOR_H
#define SCORE_HM_LOGIC_LOGIC_MONITOR_H

#include "score/mw/health/common.h"
#include "score/mw/health/tag.h"
#include <score/expected.hpp>
#include <utility>
#include <vector>

namespace score::mw::health
{
// Forward declaration
class HealthMonitor;
class HealthMonitorBuilder;
}  // namespace score::mw::health

namespace score::mw::health::logic
{

class LogicMonitorBuilder final : public internal::RustDroppable<LogicMonitorBuilder>
{
  public:
    /// Create a new `LogicMonitorBuilder`.
    ///
    /// - `initial_state` - starting point.
    LogicMonitorBuilder(const StateTag& initial_state);

    LogicMonitorBuilder(const LogicMonitorBuilder&) = delete;
    LogicMonitorBuilder& operator=(const LogicMonitorBuilder&) = delete;

    LogicMonitorBuilder(LogicMonitorBuilder&&) = default;
    LogicMonitorBuilder& operator=(LogicMonitorBuilder&&) = delete;

    /// Add state along with allowed transitions.
    /// If state already exists - it is overwritten.
    LogicMonitorBuilder AddState(const StateTag& state, const std::vector<StateTag>& allowed_states) &&;

    /// @deprecated Use `AddState()` instead. Removed in the release after v0.10.
    [[deprecated("Use AddState() instead. The snake_case API is removed in the release after v0.10.")]]
    LogicMonitorBuilder add_state(const StateTag& state, const std::vector<StateTag>& allowed_states) &&
    {
        return std::move(*this).AddState(state, allowed_states);
    }

  protected:
    std::optional<internal::FFIHandle> DropByRustImpl()
    {
        return monitor_builder_handle_.DropByRust();
    }

  private:
    internal::DroppableFFIHandle monitor_builder_handle_;

    // Allow to hide DropByRust implementation
    friend class internal::RustDroppable<LogicMonitorBuilder>;

    // Allow HealthMonitorBuilder to access DropByRust implementation
    friend class ::score::mw::health::HealthMonitorBuilder;
};

class LogicMonitor final
{
  public:
    LogicMonitor(const LogicMonitor&) = delete;
    LogicMonitor& operator=(const LogicMonitor&) = delete;

    LogicMonitor(LogicMonitor&& other) noexcept = default;
    LogicMonitor& operator=(LogicMonitor&& other) noexcept = default;

    /// Perform transition to a new state.
    /// On success, current state is returned.
    score::cpp::expected<StateTag, Error> Transition(const StateTag& state);

    /// Current monitor state.
    score::cpp::expected<StateTag, Error> State();

    /// @deprecated Use `Transition()` instead. Removed in the release after v0.10.
    [[deprecated("Use Transition() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<StateTag, Error> transition(const StateTag& state)
    {
        return Transition(state);
    }

    /// @deprecated Use `State()` instead. Removed in the release after v0.10.
    [[deprecated("Use State() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<StateTag, Error> state()
    {
        return State();
    }

  private:
    explicit LogicMonitor(internal::FFIHandle monitor_handle);

    // Only `HealthMonitor` is allowed to create `LogicMonitor` instances.
    friend class score::mw::health::HealthMonitor;
    internal::DroppableFFIHandle monitor_handle_;
};

}  // namespace score::mw::health::logic

#endif  // SCORE_HM_LOGIC_LOGIC_MONITOR_H

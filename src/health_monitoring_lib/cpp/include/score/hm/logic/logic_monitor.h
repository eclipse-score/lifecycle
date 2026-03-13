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

#include "score/hm/common.h"
#include "score/hm/tag.h"
#include <score/expected.hpp>
#include <vector>

namespace score::hm
{
// Forward declaration
class HealthMonitor;
class HealthMonitorBuilder;
}  // namespace score::hm

namespace score::hm::logic
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
    LogicMonitorBuilder add_state(const StateTag& state, const std::vector<StateTag>& allowed_states) &&;

  protected:
    std::optional<internal::FFIHandle> _drop_by_rust_impl()
    {
        return monitor_builder_handle_.drop_by_rust();
    }

  private:
    internal::DroppableFFIHandle monitor_builder_handle_;

    // Allow to hide drop_by_rust implementation
    friend class internal::RustDroppable<LogicMonitorBuilder>;

    // Allow HealthMonitorBuilder to access drop_by_rust implementation
    friend class ::score::hm::HealthMonitorBuilder;
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
    score::cpp::expected<StateTag, Error> transition(const StateTag& state);

    /// Current monitor state.
    score::cpp::expected<StateTag, Error> state();

  private:
    explicit LogicMonitor(internal::FFIHandle monitor_handle);

    // Only `HealthMonitor` is allowed to create `LogicMonitor` instances.
    friend class score::hm::HealthMonitor;
    internal::DroppableFFIHandle monitor_handle_;
};

}  // namespace score::hm::logic

#endif  // SCORE_HM_LOGIC_LOGIC_MONITOR_H

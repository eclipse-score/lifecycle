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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_MONITOR_EVALUATOR_H
#define SCORE_HEALTH_MONITOR_INTERNAL_MONITOR_EVALUATOR_H

#include <chrono>

namespace score::health_monitor::details
{

/// @brief Monitor that the worker thread checks once per internal processing cycle.
class MonitorEvaluator
{
  public:
    MonitorEvaluator(const MonitorEvaluator&) = delete;
    MonitorEvaluator(MonitorEvaluator&&) = delete;
    virtual ~MonitorEvaluator() = default;
    MonitorEvaluator& operator=(const MonitorEvaluator&) = delete;
    MonitorEvaluator& operator=(MonitorEvaluator&&) = delete;

    /// @brief Checks the monitor and logs each failure with the monitor tag.
    /// @param hmon_start Time at which the worker thread started, the same value for every call.
    /// @return `false` if the monitor failed.
    /// @pre Only the worker thread calls it.
    [[nodiscard]] virtual bool Evaluate(const std::chrono::steady_clock::time_point hmon_start) noexcept = 0;

  protected:
    MonitorEvaluator() = default;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_MONITOR_EVALUATOR_H

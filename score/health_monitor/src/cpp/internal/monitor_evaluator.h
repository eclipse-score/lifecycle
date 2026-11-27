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

#include "score/health_monitor/tag.h"

#include <chrono>
#include <cstdint>
#include <variant>

namespace score::health_monitor::details
{

/// @brief Failure of a heartbeat monitor in one evaluation.
enum class HeartbeatEvaluationError : std::uint8_t
{
    /// @brief The heartbeat came before the range of its cycle.
    kTooEarly,
    /// @brief The heartbeat came after the range of its cycle, or no heartbeat came before the range ended.
    kTooLate,
    /// @brief More than one heartbeat came in one cycle.
    kMultipleHeartbeats,
};

/// @brief Failure of a monitor in one evaluation, with one alternative per monitor kind.
using MonitorEvaluationError = std::variant<HeartbeatEvaluationError>;

/// @brief Receives the failures that `MonitorEvaluator::Evaluate()` finds.
class EvaluationErrorSink
{
  public:
    EvaluationErrorSink(const EvaluationErrorSink&) = delete;
    EvaluationErrorSink(EvaluationErrorSink&&) = delete;
    virtual ~EvaluationErrorSink() = default;
    EvaluationErrorSink& operator=(const EvaluationErrorSink&) = delete;
    EvaluationErrorSink& operator=(EvaluationErrorSink&&) = delete;

    /// @brief Receives one failure of the monitor named `tag`.
    virtual void OnError(const MonitorTag& tag, const MonitorEvaluationError& error) noexcept = 0;

  protected:
    EvaluationErrorSink() = default;
};

/// @brief Monitor that the worker thread checks once per internal processing cycle.
class MonitorEvaluator
{
  public:
    MonitorEvaluator(const MonitorEvaluator&) = delete;
    MonitorEvaluator(MonitorEvaluator&&) = delete;
    virtual ~MonitorEvaluator() = default;
    MonitorEvaluator& operator=(const MonitorEvaluator&) = delete;
    MonitorEvaluator& operator=(MonitorEvaluator&&) = delete;

    /// @brief Checks the monitor and passes each failure to `sink`.
    /// @param hmon_start Time at which the worker thread started, the same value for every call.
    /// @pre Only the worker thread calls it.
    virtual void Evaluate(
        const std::chrono::steady_clock::time_point hmon_start,
        EvaluationErrorSink& sink) noexcept = 0;

  protected:
    MonitorEvaluator() = default;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_MONITOR_EVALUATOR_H

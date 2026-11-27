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

#ifndef SCORE_HEALTH_MONITOR_TIME_RANGE_H
#define SCORE_HEALTH_MONITOR_TIME_RANGE_H

#include <score/assert.hpp>

#include <chrono>

namespace score::health_monitor
{

/// @brief Closed interval `[min, max]` of whole milliseconds.
class TimeRange final
{
  public:
    /// @brief Creates the interval `[min, max]`.
    /// @pre `0 <= min` and `min <= max`.
    /// A violation aborts through the futurecpp precondition handler.
    TimeRange(const std::chrono::milliseconds min, const std::chrono::milliseconds max) noexcept : min_{min}, max_{max}
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(min_.count() >= 0);
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(min_ <= max_);
    }

    /// @brief Returns the lower bound.
    std::chrono::milliseconds Min() const noexcept
    {
        return min_;
    }

    /// @brief Returns the upper bound.
    std::chrono::milliseconds Max() const noexcept
    {
        return max_;
    }

  private:
    /// @brief Lower bound.
    std::chrono::milliseconds min_;
    /// @brief Upper bound.
    std::chrono::milliseconds max_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_TIME_RANGE_H

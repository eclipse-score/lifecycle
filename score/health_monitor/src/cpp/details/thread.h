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
#ifndef SCORE_HM_THREAD_H
#define SCORE_HM_THREAD_H

#include "score/mw/health/common.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace score::mw::health
{

class HealthMonitorBuilder;

/// Scheduler policy.
enum class SchedulerPolicy : int32_t
{
    Other,
    Fifo,
    RoundRobin,
};

/// Get min thread priority for given policy.
int32_t SchedulerPolicyPriorityMin(SchedulerPolicy scheduler_policy);

/// Get max thread priority for given policy.
int32_t SchedulerPolicyPriorityMax(SchedulerPolicy scheduler_policy);

/// @deprecated Use `SchedulerPolicyPriorityMin()` instead. Removed in the release after v0.10.
[[deprecated("Use SchedulerPolicyPriorityMin() instead. The snake_case API is removed in the release after v0.10.")]]
inline int32_t scheduler_policy_priority_min(SchedulerPolicy scheduler_policy)
{
    return SchedulerPolicyPriorityMin(scheduler_policy);
}

/// @deprecated Use `SchedulerPolicyPriorityMax()` instead. Removed in the release after v0.10.
[[deprecated("Use SchedulerPolicyPriorityMax() instead. The snake_case API is removed in the release after v0.10.")]]
inline int32_t scheduler_policy_priority_max(SchedulerPolicy scheduler_policy)
{
    return SchedulerPolicyPriorityMax(scheduler_policy);
}

class SchedulerParameters final
{
  public:
    /// Create a new `SchedulerParameters`.
    /// Priority must be in allowed range for the scheduler policy.
    SchedulerParameters(SchedulerPolicy policy, int32_t priority);

    /// Scheduler policy.
    SchedulerPolicy Policy() const;

    /// Thread priority.
    int32_t Priority() const;

    /// @deprecated Use `Policy()` instead. Removed in the release after v0.10.
    [[deprecated("Use Policy() instead. The snake_case API is removed in the release after v0.10.")]]
    SchedulerPolicy policy() const
    {
        return Policy();
    }

    /// @deprecated Use `Priority()` instead. Removed in the release after v0.10.
    [[deprecated("Use Priority() instead. The snake_case API is removed in the release after v0.10.")]]
    int32_t priority() const
    {
        return Priority();
    }

  private:
    SchedulerPolicy policy_;
    int32_t priority_;
};

/// Thread parameters.
class ThreadParameters final : public internal::RustDroppable<ThreadParameters>
{
  public:
    /// Create a new `ThreadParameters` containing default values.
    ThreadParameters();

    /// Scheduler parameters, including scheduler policy and thread priority.
    ThreadParameters WithSchedulerParameters(SchedulerParameters scheduler_parameters) &&;

    /// Set thread affinity - array of CPU core IDs that the thread can run on.
    ThreadParameters Affinity(const std::vector<size_t>& affinity) &&;

    /// Set stack size.
    ThreadParameters StackSize(size_t stack_size) &&;

    /// @deprecated Use `WithSchedulerParameters()` instead. Removed in the release after v0.10.
    [[deprecated("Use WithSchedulerParameters() instead. The snake_case API is removed in the release after v0.10.")]]
    ThreadParameters scheduler_parameters(SchedulerParameters scheduler_parameters) &&
    {
        return std::move(*this).WithSchedulerParameters(scheduler_parameters);
    }

    /// @deprecated Use `Affinity()` instead. Removed in the release after v0.10.
    [[deprecated("Use Affinity() instead. The snake_case API is removed in the release after v0.10.")]]
    ThreadParameters affinity(const std::vector<size_t>& affinity) &&
    {
        return std::move(*this).Affinity(affinity);
    }

    /// @deprecated Use `StackSize()` instead. Removed in the release after v0.10.
    [[deprecated("Use StackSize() instead. The snake_case API is removed in the release after v0.10.")]]
    ThreadParameters stack_size(size_t stack_size) &&
    {
        return std::move(*this).StackSize(stack_size);
    }

  protected:
    std::optional<internal::FFIHandle> DropByRustImpl()
    {
        return thread_parameters_handle_.DropByRust();
    }

  private:
    internal::DroppableFFIHandle thread_parameters_handle_;

    // Allow to hide `DropByRust` implementation.
    friend class internal::RustDroppable<ThreadParameters>;

    // Allow `HealthMonitorBuilder` to access `DropByRust` implementation.
    friend class score::mw::health::HealthMonitorBuilder;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_THREAD_H

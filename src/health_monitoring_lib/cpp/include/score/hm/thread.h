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

#include "common.h"
#include <cstdint>
#include <vector>

namespace score::hm
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
int32_t scheduler_policy_priority_min(SchedulerPolicy scheduler_policy);

/// Get max thread priority for given policy.
int32_t scheduler_policy_priority_max(SchedulerPolicy scheduler_policy);

class SchedulerParameters final
{
  public:
    /// Create a new `SchedulerParameters`.
    /// Priority must be in allowed range for the scheduler policy.
    SchedulerParameters(SchedulerPolicy policy, int32_t priority);

    /// Scheduler policy.
    SchedulerPolicy policy() const;

    /// Thread priority.
    int32_t priority() const;

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
    ThreadParameters scheduler_parameters(SchedulerParameters scheduler_parameters) &&;

    /// Set thread affinity - array of CPU core IDs that the thread can run on.
    ThreadParameters affinity(const std::vector<size_t>& affinity) &&;

    /// Set stack size.
    ThreadParameters stack_size(size_t stack_size) &&;

  protected:
    std::optional<internal::FFIHandle> _drop_by_rust_impl()
    {
        return thread_parameters_handle_.drop_by_rust();
    }

  private:
    internal::DroppableFFIHandle thread_parameters_handle_;

    // Allow to hide `drop_by_rust` implementation.
    friend class internal::RustDroppable<ThreadParameters>;

    // Allow `HealthMonitorBuilder` to access `drop_by_rust` implementation.
    friend class score::hm::HealthMonitorBuilder;
};

}  // namespace score::hm

#endif  // SCORE_HM_THREAD_H

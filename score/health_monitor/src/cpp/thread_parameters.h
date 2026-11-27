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

#ifndef SCORE_HEALTH_MONITOR_THREAD_PARAMETERS_H
#define SCORE_HEALTH_MONITOR_THREAD_PARAMETERS_H

#include <score/span.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace score::health_monitor
{

/// @brief Scheduling policy of the worker thread.
enum class SchedulerPolicy : std::int32_t
{
    /// @brief Time-sharing scheduling, the default of the operating system.
    kOther = 0,

    /// @brief Real-time first-in first-out scheduling.
    kFifo = 1,

    /// @brief Real-time round-robin scheduling.
    kRoundRobin = 2,
};

/// @brief Returns the lowest priority the operating system accepts for `policy`.
std::int32_t SchedulerPolicyPriorityMin(const SchedulerPolicy policy) noexcept;

/// @brief Returns the highest priority the operating system accepts for `policy`.
std::int32_t SchedulerPolicyPriorityMax(const SchedulerPolicy policy) noexcept;

/// @brief Scheduling policy and priority of the worker thread.
class SchedulerParameters final
{
  public:
    /// @brief Sets the policy and its priority.
    /// @note `HealthMonitorBuilder::Build()` rejects a priority outside the range of `policy`.
    constexpr SchedulerParameters(const SchedulerPolicy policy, const std::int32_t priority) noexcept
        : policy_{policy}, priority_{priority}
    {
    }

    /// @brief Returns the scheduling policy.
    constexpr SchedulerPolicy Policy() const noexcept
    {
        return policy_;
    }

    /// @brief Returns the priority.
    constexpr std::int32_t Priority() const noexcept
    {
        return priority_;
    }

  private:
    /// @brief Scheduling policy.
    SchedulerPolicy policy_;
    /// @brief Priority in the range of `policy_`.
    std::int32_t priority_;
};

/// @brief Scheduling, CPU affinity and stack size of the worker thread.
/// @note A value that is not set keeps the operating system default.
/// The worker then inherits the scheduling of the thread that calls `HealthMonitor::Start()`.
class ThreadParameters final
{
  public:
    /// @brief Sets the scheduling policy and priority of the worker.
    ThreadParameters WithSchedulerParameters(const SchedulerParameters scheduler_parameters) && noexcept;

    /// @brief Restricts the worker to the CPUs in `cpus`.
    /// @param cpus CPU indices the worker can run on, each below the CPU count. The parameters keep a copy.
    /// @warning On QNX, the CPUs must form a cluster that the system defines.
    /// Otherwise the worker cannot apply the affinity and terminates the process.
    ThreadParameters WithAffinity(const score::cpp::span<const std::size_t> cpus) && noexcept;

    /// @brief Sets the stack size of the worker.
    /// @param stack_size Stack size in bytes, not below the minimum of the operating system.
    ThreadParameters WithStackSize(const std::size_t stack_size) && noexcept;

    /// @brief Returns the scheduling parameters, or no value for the operating system default.
    std::optional<SchedulerParameters> GetSchedulerParameters() const noexcept;

    /// @brief Returns the CPU indices of the worker.
    /// @return A view that stays valid while the parameters exist.
    /// An empty view keeps the operating system default.
    score::cpp::span<const std::size_t> GetAffinity() const& noexcept;

    /// @brief Deleted, because the view into a temporary dangles.
    score::cpp::span<const std::size_t> GetAffinity() const&& = delete;

    /// @brief Returns the stack size in bytes, or no value for the operating system default.
    std::optional<std::size_t> GetStackSize() const noexcept;

  private:
    /// @brief Scheduling of the worker. No value keeps the operating system default.
    std::optional<SchedulerParameters> scheduler_parameters_;
    /// @brief CPU indices of the worker. An empty list keeps the operating system default.
    std::vector<std::size_t> affinity_;
    /// @brief Stack size in bytes. No value keeps the operating system default.
    std::optional<std::size_t> stack_size_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_THREAD_PARAMETERS_H

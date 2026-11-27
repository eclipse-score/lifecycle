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

#include "score/health_monitor/src/cpp/internal/thread_setup.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/logger.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#if defined(__QNX__)
#include <sys/neutrino.h>
#endif

#include <score/span.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>

namespace score::health_monitor::details
{

namespace
{

/// @brief Largest CPU count the affinity mask holds.
const std::size_t kMaxCpuCount{1024U};

std::size_t OnlineCpuCount() noexcept
{
    const std::int64_t count{::sysconf(_SC_NPROCESSORS_ONLN)};
    return (count > 0) ? static_cast<std::size_t>(count) : 0U;
}

void SetAffinity(const score::cpp::span<const std::size_t> cpus) noexcept
{
#if defined(__QNX__)
    // ThreadCtl() data: the word count, then the run mask, then the inherit mask, each `kMaskWords` long at most.
    // A zero inherit mask leaves the mask of the threads that the worker creates unchanged.
    const std::size_t kMaskWords{kMaxCpuCount / 32U};
    std::array<std::uint32_t, 1U + (2U * kMaskWords)> data{};
    // At least one word, and never more words than `data` holds.
    const std::size_t words{std::clamp<std::size_t>((OnlineCpuCount() + 31U) / 32U, 1U, kMaskWords)};
    data[0] = static_cast<std::uint32_t>(words);
    for (const std::size_t cpu : cpus)
    {
        data[1U + (cpu / 32U)] |= (1U << (cpu % 32U));
    }

    if (::ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT, data.data()) == -1)
    {
        GetLogger().LogFatal() << "ThreadCtl failed to set the CPU affinity of the worker thread";
        std::terminate();
    }
#else
    // Value-initialization empties the set, as `CPU_ZERO` does.
    cpu_set_t cpu_set{};
    for (const std::size_t cpu : cpus)
    {
        CPU_SET(cpu, &cpu_set);
    }

    // Zero selects the calling thread.
    if (::sched_setaffinity(0, sizeof(cpu_set), &cpu_set) != 0)
    {
        GetLogger().LogFatal() << "sched_setaffinity failed to set the CPU affinity of the worker thread";
        std::terminate();
    }
#endif
}

}  // namespace

std::int32_t NativeSchedulerPolicy(const SchedulerPolicy policy) noexcept
{
    switch (policy)
    {
        case SchedulerPolicy::kFifo:
            return SCHED_FIFO;
        case SchedulerPolicy::kRoundRobin:
            return SCHED_RR;
        case SchedulerPolicy::kOther:
        default:
            return SCHED_OTHER;
    }
}

score::Result<void> ValidateThreadParameters(const ThreadParameters& thread_parameters) noexcept
{
    const std::optional<SchedulerParameters> scheduler_parameters{thread_parameters.GetSchedulerParameters()};
    if (scheduler_parameters.has_value())
    {
        const std::int32_t priority{scheduler_parameters->Priority()};
        const std::int32_t min{SchedulerPolicyPriorityMin(scheduler_parameters->Policy())};
        const std::int32_t max{SchedulerPolicyPriorityMax(scheduler_parameters->Policy())};
        if ((priority < min) || (priority > max))
        {
            GetLogger().LogError() << "Priority" << priority
                                   << "is not in allowed range for the scheduler policy:" << min << "to" << max;
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }

    const std::optional<std::size_t> stack_size{thread_parameters.GetStackSize()};
    if (stack_size.has_value() && (*stack_size < static_cast<std::size_t>(PTHREAD_STACK_MIN)))
    {
        GetLogger().LogError() << "Stack size" << *stack_size << "is below the minimum"
                               << static_cast<std::size_t>(PTHREAD_STACK_MIN);
        return score::MakeUnexpected(ErrorCode::kInvalidArgument);
    }

    const std::size_t cpu_count{OnlineCpuCount()};
    for (const std::size_t cpu : thread_parameters.GetAffinity())
    {
        if ((cpu >= cpu_count) || (cpu >= kMaxCpuCount))
        {
            GetLogger().LogError() << "CPU ID provided to affinity exceeds the number of CPUs, provided:" << cpu
                                   << "CPU count:" << cpu_count;
            return score::MakeUnexpected(ErrorCode::kInvalidArgument);
        }
    }

    return {};
}

void ApplyThreadParameters(const ThreadParameters& thread_parameters) noexcept
{
    const std::optional<SchedulerParameters> scheduler_parameters{thread_parameters.GetSchedulerParameters()};
    if (scheduler_parameters.has_value())
    {
        sched_param parameter{};
        parameter.sched_priority = scheduler_parameters->Priority();
        const std::int32_t result{::pthread_setschedparam(
            ::pthread_self(), NativeSchedulerPolicy(scheduler_parameters->Policy()), &parameter)};
        if (result != 0)
        {
            GetLogger().LogFatal() << "pthread_setschedparam failed for the worker thread, error:" << result;
            std::terminate();
        }
    }

    const score::cpp::span<const std::size_t> cpus{thread_parameters.GetAffinity()};
    if (!cpus.empty())
    {
        SetAffinity(cpus);
    }
}

}  // namespace score::health_monitor::details

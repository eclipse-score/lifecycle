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

#include "score/health_monitor/thread_parameters.h"

#include "score/health_monitor/src/cpp/internal/thread_setup.h"

#include <sched.h>

#include <utility>

namespace score::health_monitor
{

std::int32_t SchedulerPolicyPriorityMin(const SchedulerPolicy policy) noexcept
{
    return ::sched_get_priority_min(details::NativeSchedulerPolicy(policy));
}

std::int32_t SchedulerPolicyPriorityMax(const SchedulerPolicy policy) noexcept
{
    return ::sched_get_priority_max(details::NativeSchedulerPolicy(policy));
}

ThreadParameters ThreadParameters::WithSchedulerParameters(const SchedulerParameters scheduler_parameters) && noexcept
{
    scheduler_parameters_ = scheduler_parameters;
    return std::move(*this);
}

ThreadParameters ThreadParameters::WithAffinity(const score::cpp::span<const std::size_t> cpus) && noexcept
{
    affinity_.assign(cpus.begin(), cpus.end());
    return std::move(*this);
}

ThreadParameters ThreadParameters::WithStackSize(const std::size_t stack_size) && noexcept
{
    stack_size_ = stack_size;
    return std::move(*this);
}

std::optional<SchedulerParameters> ThreadParameters::GetSchedulerParameters() const noexcept
{
    return scheduler_parameters_;
}

score::cpp::span<const std::size_t> ThreadParameters::GetAffinity() const& noexcept
{
    return {affinity_.data(), affinity_.size()};
}

std::optional<std::size_t> ThreadParameters::GetStackSize() const noexcept
{
    return stack_size_;
}

}  // namespace score::health_monitor

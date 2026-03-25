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

#include "score/hm/thread.h"
#include "score/hm/common.h"
#include <score/assert.hpp>

namespace
{
extern "C" {

using namespace score::hm;
using namespace score::hm::internal;

// Functions below must match functions defined in `crate::ffi`.

FFICode scheduler_policy_priority_min(SchedulerPolicy scheduler_policy, int32_t* priority_out);
FFICode scheduler_policy_priority_max(SchedulerPolicy scheduler_policy, int32_t* priority_out);
}
}  // namespace

namespace score::hm
{

int32_t scheduler_policy_priority_min(SchedulerPolicy scheduler_policy)
{
    int32_t priority{0};
    auto result{::scheduler_policy_priority_min(scheduler_policy, &priority)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return priority;
}

int32_t scheduler_policy_priority_max(SchedulerPolicy scheduler_policy)
{
    int32_t priority{0};
    auto result{::scheduler_policy_priority_max(scheduler_policy, &priority)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return priority;
}

SchedulerParameters::SchedulerParameters(SchedulerPolicy policy, int32_t priority)
    : policy_{policy}, priority_{priority}
{
    auto min{scheduler_policy_priority_min(policy)};
    auto max{scheduler_policy_priority_max(policy)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(priority >= min && priority <= max);
}

const SchedulerPolicy& SchedulerParameters::policy() const
{
    return policy_;
}

const int32_t& SchedulerParameters::priority() const
{
    return priority_;
}

ThreadParameters ThreadParameters::scheduler_parameters(SchedulerParameters scheduler_parameters) &&
{
    scheduler_parameters_ = scheduler_parameters;
    return std::move(*this);
}

ThreadParameters ThreadParameters::affinity(const std::vector<size_t>& affinity) &&
{
    affinity_ = affinity;
    return std::move(*this);
}

ThreadParameters ThreadParameters::stack_size(size_t stack_size) &&
{
    stack_size_ = stack_size;
    return std::move(*this);
}

}  // namespace score::hm

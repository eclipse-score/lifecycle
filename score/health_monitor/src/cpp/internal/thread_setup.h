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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_THREAD_SETUP_H
#define SCORE_HEALTH_MONITOR_INTERNAL_THREAD_SETUP_H

#include "score/health_monitor/thread_parameters.h"
#include "score/result/result.h"

#include <cstdint>

namespace score::health_monitor::details
{

/// @brief Returns the `SCHED_*` value of `policy`.
std::int32_t NativeSchedulerPolicy(const SchedulerPolicy policy) noexcept;

/// @brief Checks the priority, stack size and CPU indices against the limits of the operating system.
/// @return An empty result, or `ErrorCode::kInvalidArgument` if one of them is outside its limit.
/// @note The priority is in the range of the policy, and the stack is at least `PTHREAD_STACK_MIN`.
/// @note A CPU index is below the online CPU count and below 1024.
score::Result<void> ValidateThreadParameters(const ThreadParameters& thread_parameters) noexcept;

/// @brief Applies the scheduling policy, priority and CPU affinity to the calling thread.
/// @pre `ValidateThreadParameters()` accepted the parameters.
/// @note A failure terminates the process, for example a real-time policy without the privilege to use it.
/// @note The stack size is not applied, because only thread creation can set it.
void ApplyThreadParameters(const ThreadParameters& thread_parameters) noexcept;

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_THREAD_SETUP_H

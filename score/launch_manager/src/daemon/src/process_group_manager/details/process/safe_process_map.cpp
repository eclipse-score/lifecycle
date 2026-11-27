/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include "score/mw/launch_manager/process_group_manager/details/process/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include <score/assert.hpp>

namespace score::mw::lifecycle::internal
{

SafeProcessMap::SafeProcessMap(size_t capacity, IComponentController& termination_handler)
    : capacity_(capacity),
      pool_resource_(&init_resource_),
      process_map_(&pool_resource_),
      termination_handler_(termination_handler)
{
    // Reserve enough memory to store the full capacity.
    process_map_.reserve(capacity_);
    for (size_t i = 0; i < capacity_; ++i)
    {
        process_map_.emplace(static_cast<osal::ProcessID>(i), int32_t{});
    }
    process_map_.clear();

    // Guarantee no further allocations happen at runtime.
    init_resource_.lock();
}

SafeProcessMapReturnType SafeProcessMap::findTerminated(osal::ProcessID key, int32_t status)
{
    const TryInsertResult<std::reference_wrapper<IComponent>> result =
        tryInsert<int32_t, std::reference_wrapper<IComponent>>(key, status);

    // The handler is invoked without holding the map lock. Components insert themselves while
    // holding their own lock, and the handler takes that lock, so invoking it under the map lock
    // could deadlock.
    if (result.match.has_value())
    {
        termination_handler_.terminated(result.match->get(), ProcessHandle{key, nullptr}, status);
    }

    return result.type;
}

SafeProcessMapInsertResult SafeProcessMap::insertIfNotTerminated(osal::ProcessID key, IComponent* component)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT(component != nullptr);

    const TryInsertResult<int32_t> result =
        tryInsert<std::reference_wrapper<IComponent>, int32_t>(key, std::ref(*component));

    return SafeProcessMapInsertResult{result.type, result.match.value_or(0)};
}

}  // namespace score::mw::lifecycle::internal

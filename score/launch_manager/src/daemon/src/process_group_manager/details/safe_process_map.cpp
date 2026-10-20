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

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"

namespace score::mw::lifecycle::internal
{

SafeProcessMap::SafeProcessMap(size_t capacity, IComponentController& termination_handler)
    : capacity_(capacity), termination_handler_(termination_handler)
{
    process_map_.reserve(capacity_);
}

SafeProcessMapReturnType SafeProcessMap::findTerminated(osal::ProcessID key, int32_t status)
{
    if (key < 0)
    {
        return SafeProcessMapReturnType::kInvalidIdError;
    }

    std::unique_lock map_lock(map_mutex_);

    while (true)
    {
        const auto it = process_map_.find(key);

        if (it != process_map_.end())
        {
            if (auto* component = std::get_if<std::reference_wrapper<IComponent>>(&it->second))
            {
                IComponent& matched_component = component->get();
                process_map_.erase(it);
                map_cv_.notify_all();
                termination_handler_.terminated(matched_component, status);
                return SafeProcessMapReturnType::kOk;
            }
        }
        else
        {
            return storeExitCode(key, status);
        }

        // If there is no match, and there is already an exit code in the map, then we
        // are dealing with a reused ID. This is handled by blocking until the previous
        // entry is removed.
        map_cv_.wait(map_lock);
    }
}

SafeProcessMapReturnType SafeProcessMap::insertIfNotTerminated(osal::ProcessID key, IComponent* component)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT(component != nullptr);

    if (key < 0)
    {
        return SafeProcessMapReturnType::kInvalidIdError;
    }

    std::unique_lock map_lock(map_mutex_);

    while (true)
    {
        const auto it = process_map_.find(key);

        if (it != process_map_.end())
        {
            if (const auto* exit_code = std::get_if<int32_t>(&it->second))
            {
                const int32_t matched_exit_code = *exit_code;
                process_map_.erase(it);
                map_cv_.notify_all();
                termination_handler_.terminated(*component, matched_exit_code);
                return SafeProcessMapReturnType::kYield;
            }
        }
        else
        {
            return storeComponent(key, *component);
        }

        // If there is no match, and there is already a component in the map, then we
        // are dealing with a reused ID. This is handled by blocking until the previous
        // entry is removed.
        map_cv_.wait(map_lock);
    }
}

SafeProcessMapReturnType SafeProcessMap::storeComponent(osal::ProcessID key, IComponent& component)
{
    if (process_map_.size() == capacity_)
    {
        return SafeProcessMapReturnType::kInsertionError;
    }

    const bool inserted = process_map_.try_emplace(key, std::ref(component)).second;
    return inserted ? SafeProcessMapReturnType::kOk : SafeProcessMapReturnType::kInsertionError;
}

SafeProcessMapReturnType SafeProcessMap::storeExitCode(osal::ProcessID key, int32_t status)
{
    if (process_map_.size() == capacity_)
    {
        return SafeProcessMapReturnType::kInsertionError;
    }

    const bool inserted = process_map_.try_emplace(key, status).second;
    return inserted ? SafeProcessMapReturnType::kYield : SafeProcessMapReturnType::kInsertionError;
}

}  // namespace score::mw::lifecycle::internal

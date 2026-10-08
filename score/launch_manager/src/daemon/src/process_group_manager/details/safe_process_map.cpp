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
    component_map_.reserve(capacity_);
    exit_code_map_.reserve(capacity_);
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
        if (const std::optional<std::reference_wrapper<IComponent>> matched = matchComponent(key))
        {
            termination_handler_.terminated(matched->get(), status);
            return SafeProcessMapReturnType::kOk;
        }

        if (exit_code_map_.count(key) == 0)
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
        if (const std::optional<int32_t> exit_code = matchExitCode(key))
        {
            termination_handler_.terminated(*component, *exit_code);
            return SafeProcessMapReturnType::kYield;
        }

        if (component_map_.count(key) == 0)
        {
            return storeComponent(key, *component);
        }

        // If there is no match, and there is already a component in the map, then we
        // are dealing with a reused ID. This is handled by blocking until the previous
        // entry is removed.
        map_cv_.wait(map_lock);
    }
}

std::optional<std::reference_wrapper<IComponent>> SafeProcessMap::matchComponent(osal::ProcessID key)
{
    const auto it = component_map_.find(key);

    if (it == component_map_.end())
    {
        return std::nullopt;
    }

    const std::reference_wrapper<IComponent> component = it->second;
    component_map_.erase(it);
    map_cv_.notify_all();
    return component;
}

SafeProcessMapReturnType SafeProcessMap::storeComponent(osal::ProcessID key, IComponent& component)
{
    if (component_map_.size() == capacity_)
    {
        return SafeProcessMapReturnType::kInsertionError;
    }

    const bool inserted = component_map_.try_emplace(key, component).second;
    return inserted ? SafeProcessMapReturnType::kOk : SafeProcessMapReturnType::kInsertionError;
}

std::optional<int32_t> SafeProcessMap::matchExitCode(osal::ProcessID key)
{
    const auto it = exit_code_map_.find(key);

    if (it == exit_code_map_.end())
    {
        return std::nullopt;
    }

    const int32_t exit_code = it->second;
    exit_code_map_.erase(it);
    map_cv_.notify_all();
    return exit_code;
}

SafeProcessMapReturnType SafeProcessMap::storeExitCode(osal::ProcessID key, int32_t status)
{
    if (exit_code_map_.size() == capacity_)
    {
        return SafeProcessMapReturnType::kInsertionError;
    }

    const bool inserted = exit_code_map_.try_emplace(key, status).second;
    return inserted ? SafeProcessMapReturnType::kYield : SafeProcessMapReturnType::kInsertionError;
}

}  // namespace score::mw::lifecycle::internal

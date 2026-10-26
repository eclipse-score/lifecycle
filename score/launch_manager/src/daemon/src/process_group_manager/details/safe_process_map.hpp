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

#ifndef SAFE_PROCESS_MAP_HPP_INCLUDED
#define SAFE_PROCESS_MAP_HPP_INCLUDED

#include "score/mw/launch_manager/common/init_only_resource.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <variant>

namespace score::mw::lifecycle::internal
{

/// @brief Enum type used for public methods return value.
enum class SafeProcessMapReturnType : std::uint_fast8_t
{
    /// @brief The termination handler was called for a matched process and component.
    kMatched,

    /// @brief An entry was inserted and is waiting for the other insertion.
    kInserted,

    /// @brief An error occurred during insertion (e.g., out of memory).
    kInsertionError,

    /// @brief The provided process ID (`key`) is not valid ( < 0).
    kInvalidIdError,
};

/// @brief Interface for inserting components into SafeProcessMap.
class SafeProcessMapInserter
{
  public:
    virtual ~SafeProcessMapInserter() = default;

    /// @brief Inserts a process into the map if it has not already terminated.
    /// @param key The process ID to look for in the map.
    /// @param component The component to notify with the exit code.
    virtual SafeProcessMapReturnType insertIfNotTerminated(osal::ProcessID key, IComponent* component) = 0;
};

/// @brief The SafeProcessMap waits for both a component and exit code to be
///          registered under the same process ID, then invokes a termination
///          handler with both values.
class SafeProcessMap final : public SafeProcessMapInserter
{
  public:
    /// @brief Constructs a SafeProcessMap.
    /// @param capacity The maximum number of entries the map can hold.
    /// @param termination_handler Called when a terminated process is matched with its component.
    SafeProcessMap(size_t capacity, IComponentController& termination_handler);

    /// @brief Destructor to clean up resources used by the SafeProcessMap object.
    ~SafeProcessMap() = default;

    /// @brief Finds a terminated process in the map.
    /// @param key The process ID to look for in the map.
    /// @param status The exit code to notify the component with.
    SafeProcessMapReturnType findTerminated(osal::ProcessID key, int32_t status);

    /// @brief Inserts a process into the map if it has not already terminated.
    /// @param key The process ID to look for in the map.
    /// @param component The component to notify with the exit code.
    SafeProcessMapReturnType insertIfNotTerminated(osal::ProcessID key, IComponent* component);

  private:
    template <typename InsertType, typename MatchType, typename OnMatch>
    SafeProcessMapReturnType tryInsert(osal::ProcessID key, InsertType value, OnMatch on_match)
    {
        if (key < 0)
        {
            return SafeProcessMapReturnType::kInvalidIdError;
        }

        std::unique_lock map_lock(map_mutex_);

        while (true)
        {
            const auto it = process_map_.find(key);

            if (it == process_map_.end())
            {
                if (process_map_.size() == capacity_)
                {
                    return SafeProcessMapReturnType::kInsertionError;
                }

                const bool inserted = process_map_.try_emplace(key, value).second;

                return inserted ? SafeProcessMapReturnType::kInserted : SafeProcessMapReturnType::kInsertionError;
            }

            if (const MatchType* match = std::get_if<MatchType>(&it->second))
            {
                process_map_.erase(it);
                map_cv_.notify_all();
                on_match(*match);
                return SafeProcessMapReturnType::kMatched;
            }

            // If there is no match, and there is already a value stored, then
            // we are dealing with a reused key. This is handled by blocking
            // until the previous entry is removed.
            map_cv_.wait(map_lock);
        }
    }

    /// @brief Maximum number of processes which can be stored simultaneously.
    size_t capacity_;

    /// @brief Resource which pre-allocates memory.
    InitOnlyResource init_resource_;

    /// @brief Resource which allows the pre-allocated memory to be reused.
    std::pmr::unsynchronized_pool_resource pool_resource_;

    /// @brief Mutex which protects both maps.
    std::mutex map_mutex_;

    /// @brief Condition variable which is signalled whenever an entry is removed.
    std::condition_variable map_cv_;

    /// @brief Map of process IDs to components or exit codes.
    std::pmr::unordered_map<osal::ProcessID, std::variant<int32_t, std::reference_wrapper<IComponent>>> process_map_;

    /// @brief Handler to notify about process terminations.
    IComponentController& termination_handler_;
};

}  // namespace score::mw::lifecycle::internal

#endif  /// SAFE_PROCESS_MAP_HPP_INCLUDED

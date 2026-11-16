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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_DEFAULT_DEFAULT_WATCHDOG_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_DEFAULT_DEFAULT_WATCHDOG_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/iwatchdog.hpp"

#include "score/memory/shared/i_shared_memory_resource.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Name of the shared memory region with the Launch Manager heartbeat.
constexpr const char* kHeartbeatShmPath{"/launch_manager_heartbeat"};

/// @brief Launch Manager heartbeat
/// @details The Launch Manager creates the region in IWatchdog::init(), writable only by its own user and readable by
/// everybody, and increments `counter` once per main loop iteration as long as its running successfully.
/// Reading it with score/memory/shared:
/// @code
/// auto resource = score::memory::shared::SharedMemoryFactory::Open(kHeartbeatShmPath, false);
/// const auto* heartbeat = static_cast<const Heartbeat*>(resource->getUsableBaseAddress());
/// const auto value = heartbeat->counter.load(std::memory_order_relaxed);
/// @endcode
struct Heartbeat
{
    std::atomic<std::uint64_t> counter{0U};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free, "The heartbeat counter must be lock-free.");
static_assert(std::is_standard_layout_v<Heartbeat>, "The heartbeat must have a stable layout.");

/// @brief Default watchdog implementation.
/// @details Does not service any hardware watchdog. Instead, it publishes a heartbeat counter in shared memory (see
/// Heartbeat), so that an external application can infer the health of the Launch Manager.
///
/// The default watchdog does not reset the machine. The Launch Manager can only detect some failures itself, so the
/// integrator is responsible for an external monitor (e.g. observing the heartbeat or the Launch Manager process)
/// that brings the system into a safe state.
class DefaultWatchdog final : public IWatchdog
{
  public:
    /// @param heartbeat_path Name of the heartbeat shared memory region.
    explicit DefaultWatchdog(std::string heartbeat_path = kHeartbeatShmPath);

    /// @brief Creates the heartbeat shared memory region. A stale region left by a previous run is removed first.
    /// @return false if the region cannot be created.
    bool init() noexcept override;

    /// @brief Increments the heartbeat counter.
    void serviceWatchdog() noexcept override;

    /// @brief Stops the heartbeat and terminates the Launch Manager process with EXIT_FAILURE. Does not return.
    /// @details The heartbeat region is left in place, so an external observer sees a frozen counter.
    void fireWatchdog() noexcept override;

    /// @brief Removes the heartbeat shared memory region.
    void deinit() noexcept override;

  private:
    std::string heartbeat_path_;
    std::shared_ptr<score::memory::shared::ISharedMemoryResource> heartbeat_resource_{};
    /// @brief Points into heartbeat_resource_. nullptr before init(), after deinit() and after fireWatchdog().
    Heartbeat* heartbeat_{nullptr};
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_DEFAULT_DEFAULT_WATCHDOG_HPP_INCLUDED

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

#ifndef SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_H
#define SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_H

#include "score/result/result.h"

#include <memory>

namespace score::health_monitor
{

/// @brief Connection to the supervisor of the process, which expects an alive notification each supervisor cycle.
class SupervisorClient
{
  public:
    SupervisorClient(const SupervisorClient&) = delete;
    SupervisorClient(SupervisorClient&&) = delete;
    virtual ~SupervisorClient() = default;
    SupervisorClient& operator=(const SupervisorClient&) = delete;
    SupervisorClient& operator=(SupervisorClient&&) = delete;

    /// @brief Creates the client of the backend that the process links.
    /// @note Each backend defines this function, and a process links exactly one backend.
    static std::unique_ptr<SupervisorClient> Create() noexcept;

    /// @brief Connects to the supervisor.
    /// @return An empty result, or `ErrorCode::kSupervisorUnavailable`, after which a later call tries again.
    /// @pre No earlier call succeeded.
    virtual score::Result<void> Connect() noexcept = 0;

    /// @brief Tells the supervisor that the process is alive.
    /// @pre `Connect()` succeeded.
    /// @pre Only one thread calls it.
    virtual void NotifyAlive() noexcept = 0;

  protected:
    SupervisorClient() = default;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_H

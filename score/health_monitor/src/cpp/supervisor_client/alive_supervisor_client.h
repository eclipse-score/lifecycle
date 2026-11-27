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

#ifndef SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_ALIVE_SUPERVISOR_CLIENT_H
#define SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_ALIVE_SUPERVISOR_CLIENT_H

#include "score/health_monitor/supervisor_client.h"
#include "score/result/result.h"

namespace score::launch_manager
{

/// @brief Supervisor client over the alive API of the launch manager.
/// @note The launch manager allows one alive client per process.
class AliveSupervisorClient final : public score::health_monitor::SupervisorClient
{
  public:
    AliveSupervisorClient() noexcept = default;
    AliveSupervisorClient(const AliveSupervisorClient&) = delete;
    AliveSupervisorClient(AliveSupervisorClient&&) = delete;
    /// @brief Closes the alive client if `Connect()` created one.
    ~AliveSupervisorClient() override;
    AliveSupervisorClient& operator=(const AliveSupervisorClient&) = delete;
    AliveSupervisorClient& operator=(AliveSupervisorClient&&) = delete;

    /// @brief Creates the alive client for the process named by the `IDENTIFIER` environment variable.
    /// @return An empty result, or `score::health_monitor::ErrorCode::kSupervisorUnavailable` if `IDENTIFIER` is not
    /// set or the alive client cannot connect to the launch manager.
    score::Result<void> Connect() noexcept override;

    void NotifyAlive() noexcept override;

  private:
    /// @brief Handle from the C API of the alive client, null until `Connect()` succeeds.
    void* alive_{nullptr};
};

}  // namespace score::launch_manager

#endif  // SCORE_HEALTH_MONITOR_SUPERVISOR_CLIENT_ALIVE_SUPERVISOR_CLIENT_H

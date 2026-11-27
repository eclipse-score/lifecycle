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

#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/supervisor_client.h"

#include <memory>

namespace score::health_monitor
{

namespace
{

/// @brief Supervisor client without a supervisor: it connects at once and only logs each alive notification.
/// @note The log runs on the worker, so a blocked log backend delays the next evaluation.
/// No supervisor reacts to a failure, and the failure log of the worker uses the same backend.
class StubSupervisorClient final : public SupervisorClient
{
  public:
    StubSupervisorClient() noexcept = default;
    StubSupervisorClient(const StubSupervisorClient&) = delete;
    StubSupervisorClient(StubSupervisorClient&&) = delete;
    ~StubSupervisorClient() override = default;
    StubSupervisorClient& operator=(const StubSupervisorClient&) = delete;
    StubSupervisorClient& operator=(StubSupervisorClient&&) = delete;

    score::Result<void> Connect() noexcept override
    {
        return {};
    }

    void NotifyAlive() noexcept override
    {
        details::GetLogger().LogWarn() << "StubSupervisorAPIClient: notify_alive called";
    }
};

}  // namespace

std::unique_ptr<SupervisorClient> SupervisorClient::Create() noexcept
{
    return std::make_unique<StubSupervisorClient>();
}

}  // namespace score::health_monitor

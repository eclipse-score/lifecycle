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

#include "score/health_monitor/src/cpp/supervisor_client/alive_supervisor_client.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/mw/lifecycle/alive.h"

#include <score/assert.hpp>

#include <cstdlib>
#include <memory>
#include <string_view>

namespace score::launch_manager
{

using score::health_monitor::ErrorCode;
using score::health_monitor::details::GetLogger;

AliveSupervisorClient::~AliveSupervisorClient()
{
    // The C API aborts on a null handle, which a client that never connected holds.
    if (alive_ != nullptr)
    {
        score_lcm_alive_deinitialize(alive_);
    }
}

score::Result<void> AliveSupervisorClient::Connect() noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(alive_ == nullptr);

    // `getenv` is not thread-safe, so `HealthMonitor::Start()` requires that no thread changes the environment.
    const char* const identifier{std::getenv("IDENTIFIER")};  // NOLINT(concurrency-mt-unsafe)
    if (identifier == nullptr)
    {
        GetLogger().LogError() << "IDENTIFIER env not set";
        return score::MakeUnexpected(ErrorCode::kSupervisorUnavailable);
    }

    GetLogger().LogDebug() << "ScoreSupervisorAPIClient: Creating with IDENTIFIER=" << std::string_view{identifier};
    // The C API returns null where the C++ constructor throws, for example without a launch manager.
    alive_ = score_lcm_alive_initialize(identifier);
    if (alive_ == nullptr)
    {
        GetLogger().LogError() << "Failed to create supervisor_link";
        return score::MakeUnexpected(ErrorCode::kSupervisorUnavailable);
    }

    return {};
}

void AliveSupervisorClient::NotifyAlive() noexcept
{
    score_lcm_alive_report_alive(alive_);
}

}  // namespace score::launch_manager

namespace score::health_monitor
{

std::unique_ptr<SupervisorClient> SupervisorClient::Create() noexcept
{
    return std::make_unique<score::launch_manager::AliveSupervisorClient>();
}

}  // namespace score::health_monitor

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

#include "score/health_monitor/src/cpp/internal/monitoring_logic.h"

#include "score/health_monitor/src/cpp/internal/logger.h"
#include "score/health_monitor/src/cpp/internal/monotonic_time.h"
#include "score/mw/log/log_stream.h"
#include "score/mw/log/logger.h"

namespace score::health_monitor::details
{

MonitoringLogic::MonitoringLogic(
    const score::cpp::span<MonitorEvaluator* const> evaluators,
    const std::chrono::milliseconds supervisor_api_cycle,
    SupervisorClient& client,
    const score::os::Time& time) noexcept
    : evaluators_{evaluators},
      supervisor_api_cycle_{supervisor_api_cycle},
      client_{client},
      time_{time},
      last_notification_{ReadMonotonicTime(time_)}
{
}

bool MonitoringLogic::Run(const std::chrono::steady_clock::time_point hmon_start) noexcept
{
    bool failed{false};
    // Every monitor runs also after a failure, so each failure of this cycle gets its log line.
    for (MonitorEvaluator* const evaluator : evaluators_)
    {
        if (!evaluator->Evaluate(hmon_start))
        {
            failed = true;
        }
    }

    if (failed)
    {
        GetLogger().LogWarn() << "One or more monitors reported errors, skipping AliveAPI notification.";
        return false;
    }

    const std::chrono::steady_clock::time_point now{ReadMonotonicTime(time_)};
    if ((now - last_notification_) > supervisor_api_cycle_)
    {
        last_notification_ = now;
        client_.NotifyAlive();
    }

    return true;
}

}  // namespace score::health_monitor::details

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

#ifndef SCORE_HEALTH_MONITOR_MOCK_HEALTH_MONITOR_H
#define SCORE_HEALTH_MONITOR_MOCK_HEALTH_MONITOR_H

#include "score/health_monitor/health_monitor.h"

#include <gmock/gmock.h>

namespace score::health_monitor::testing
{

/// @brief Mock of `HealthMonitor` for tests of code that takes a `HealthMonitor&`.
/// @pre The mocks of the returned monitors outlive every use through this mock.
/// @note Return a monitor mock with `::testing::Return(std::ref(monitor_mock))`.
class HealthMonitorMock : public HealthMonitor
{
  public:
    MOCK_METHOD(
        score::Result<Taken<DeadlineMonitor>>,
        GetDeadlineMonitor,
        (const MonitorTag& tag),
        (noexcept, override));
    MOCK_METHOD(
        score::Result<Taken<HeartbeatMonitor>>,
        GetHeartbeatMonitor,
        (const MonitorTag& tag),
        (noexcept, override));
    MOCK_METHOD(score::Result<Taken<LogicMonitor>>, GetLogicMonitor, (const MonitorTag& tag), (noexcept, override));
    MOCK_METHOD(score::Result<void>, Start, (), (noexcept, override));
};

}  // namespace score::health_monitor::testing

#endif  // SCORE_HEALTH_MONITOR_MOCK_HEALTH_MONITOR_H

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

#ifndef SCORE_HEALTH_MONITOR_MOCK_HEARTBEAT_MONITOR_H
#define SCORE_HEALTH_MONITOR_MOCK_HEARTBEAT_MONITOR_H

#include "score/health_monitor/heartbeat_monitor.h"

#include <gmock/gmock.h>

namespace score::health_monitor::testing
{

/// @brief Mock of `HeartbeatMonitor`.
class HeartbeatMonitorMock : public HeartbeatMonitor
{
  public:
    MOCK_METHOD(void, Heartbeat, (), (noexcept, override));
};

}  // namespace score::health_monitor::testing

#endif  // SCORE_HEALTH_MONITOR_MOCK_HEARTBEAT_MONITOR_H

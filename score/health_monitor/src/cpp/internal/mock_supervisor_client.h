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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_MOCK_SUPERVISOR_CLIENT_H
#define SCORE_HEALTH_MONITOR_INTERNAL_MOCK_SUPERVISOR_CLIENT_H

#include "score/health_monitor/supervisor_client.h"

#include <gmock/gmock.h>

namespace score::health_monitor::details
{

/// @brief Mock of `SupervisorClient`.
class SupervisorClientMock : public SupervisorClient
{
  public:
    MOCK_METHOD(score::Result<void>, Connect, (), (noexcept, override));
    MOCK_METHOD(void, NotifyAlive, (), (noexcept, override));
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_MOCK_SUPERVISOR_CLIENT_H

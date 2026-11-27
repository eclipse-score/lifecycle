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

#ifndef SCORE_HEALTH_MONITOR_MOCK_DEADLINE_H
#define SCORE_HEALTH_MONITOR_MOCK_DEADLINE_H

#include "score/health_monitor/deadline.h"

#include <gmock/gmock.h>

namespace score::health_monitor::testing
{

/// @brief Mock of `Deadline`. `Start()` calls `DoStart()`, and the returned handle calls `DoStop()`.
/// @note `Start()` returns `ErrorCode::kDeadlineRunning` while a handle exists, without a `DoStart()` call.
/// Inside this namespace, write gmock names as `::testing::`.
class DeadlineMock : public Deadline
{
  public:
    MOCK_METHOD(score::Result<void>, DoStart, (), (noexcept, override));
    MOCK_METHOD(void, DoStop, (), (noexcept, override));
};

}  // namespace score::health_monitor::testing

#endif  // SCORE_HEALTH_MONITOR_MOCK_DEADLINE_H

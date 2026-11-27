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

#ifndef SCORE_HEALTH_MONITOR_MOCK_TAKEN_H
#define SCORE_HEALTH_MONITOR_MOCK_TAKEN_H

#include "score/health_monitor/taken.h"

#include <memory>

namespace score::health_monitor::testing
{

/// @brief Returns a handle to `object` that owns nothing, for a mock `Get*` call to return.
/// @pre `object` outlives the handle.
template <typename T>
Taken<T> MakeTaken(T& object) noexcept
{
    // The aliasing constructor with an empty owner gives a non-null pointer without a control block.
    return Taken<T>{std::shared_ptr<T>{std::shared_ptr<void>{}, &object}};
}

}  // namespace score::health_monitor::testing

#endif  // SCORE_HEALTH_MONITOR_MOCK_TAKEN_H

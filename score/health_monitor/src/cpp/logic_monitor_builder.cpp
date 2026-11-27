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

#include "score/health_monitor/logic_monitor_builder.h"

#include <score/span.hpp>
#include <score/utility.hpp>

#include <utility>

namespace score::health_monitor
{

LogicMonitorBuilder::LogicMonitorBuilder(const StateTag& initial_state) noexcept : initial_state_{initial_state}
{
}

LogicMonitorBuilder LogicMonitorBuilder::AddState(
    const StateTag& state,
    const score::cpp::span<const StateTag> allowed_states) && noexcept
{
    score::cpp::ignore =
        states_.emplace_back(Node{state, std::vector<StateTag>{allowed_states.begin(), allowed_states.end()}});
    return std::move(*this);
}

}  // namespace score::health_monitor

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

#include "score/health_monitor/deadline_monitor_builder.h"

#include <score/utility.hpp>

#include <utility>

namespace score::health_monitor
{

DeadlineMonitorBuilder DeadlineMonitorBuilder::AddDeadline(const DeadlineTag& tag, const TimeRange& range) && noexcept
{
    score::cpp::ignore = deadlines_.emplace_back(Entry{tag, range});
    return std::move(*this);
}

}  // namespace score::health_monitor

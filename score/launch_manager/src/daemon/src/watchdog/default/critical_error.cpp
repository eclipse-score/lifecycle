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

#include "score/mw/launch_manager/watchdog/critical_error.hpp"

#include "score/mw/launch_manager/common/log.hpp"

#include <cstdlib>

namespace score::mw::lifecycle::internal::watchdog
{

void onCriticalError(CriticalErrorType type, std::string_view msg) noexcept
{
    // The heartbeat region of the DefaultWatchdog is intentionally left in place: a frozen counter tells an external
    // monitor that the Launch Manager terminated abnormally.
    LM_LOG_FATAL() << "Critical error" << toString(type) << ":" << msg << "- aborting the Launch Manager";
    std::abort();
}

}  // namespace score::mw::lifecycle::internal::watchdog

/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <algorithm>

#include "score/mw/launch_manager/alive_monitor/details/daemon/CyclicExecutor.hpp"

#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

CyclicExecutor::CyclicExecutor(OsClock& f_osClock, std::size_t supervised_components)
    : os_clock_{f_osClock},
      cycle_timer_{&os_clock_},
      buffer_(std::make_shared<SupervisionBufferType>()),
      supervision_manager_{std::make_unique<factory::AliveWorkerFactory>()},
      supervision_state_reader_{buffer_}
{
    buffer_->initialize();
    supervision_manager_.reserve(supervised_components);
}

void CyclicExecutor::performCyclicTriggers(void)
{
    std::chrono::nanoseconds syncTimestamp{timers::OsClock::getMonotonicSystemClock()};
    if (syncTimestamp.count() == 0U)
    {
        // No valid time value, use max value for synchronization
        // All received data will be considered.
        syncTimestamp = std::chrono::nanoseconds::max();
    }

    if (supervision_state_reader_.distributeChanges(syncTimestamp))
    {
        supervision_manager_.performCyclicTriggers(syncTimestamp);
    }
    else
    {
        // distributeChanges may fail due to buffer overflow,
        // which is checked on the sender side and results in a watchdog timeout.
    }
}

}  // namespace score::mw::lifecycle::internal::saf::daemon

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

#ifndef MOCK_MONITOR_IF_DAEMON
#define MOCK_MONITOR_IF_DAEMON

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/IMonitorIfDaemon.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::saf::ifappl
{
class MockMonitorIfDaemon : public IMonitorIfDaemon
{
    MOCK_METHOD(void, checkForNewData, (const std::chrono::nanoseconds f_syncTimestamp), (noexcept));
    MOCK_METHOD(void, updateData, (const ifexm::ObservableEvent& f_observable_r), (noexcept));
    MOCK_METHOD(IdentifierHash, getIdentifier, (), (const, noexcept));

  public:
    void PushCheckpointToObservers(Checkpoint checkpoint)
    {
        pushResultToObservers(checkpoint);
    }
};
}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif

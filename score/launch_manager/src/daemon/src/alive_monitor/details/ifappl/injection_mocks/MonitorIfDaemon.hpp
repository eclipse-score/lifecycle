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

#ifndef MOCK_MONITORIFDAEMON_HPP_INCLUDED
#define MOCK_MONITORIFDAEMON_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::saf::ifappl
{

struct MockMonitorIfDaemon
{
    MOCK_METHOD(IdentifierHash, getIdentifier, (), (const noexcept(true)));
    MOCK_METHOD(void, updateData, (const ifexm::ObservableEvent& f_observable_r), (noexcept(true)));
    MOCK_METHOD(void, checkForNewData, (const std::chrono::nanoseconds f_syncTimestamp), (noexcept(true)));
    // Not a real method, but we will call this from the constructor to inject exceptions
    MOCK_METHOD(void, ctor, (CheckpointIpcServer & f_ipcServer_r, const char* f_interfaceName_p), (noexcept(false)));
};

/// @brief @see tests/utils/mocks/README.md#Header%20replacement for explanation and usage
extern std::unique_ptr<MockMonitorIfDaemon> g_mock_monitor_if_daemon;

class MonitorIfDaemon : public common::Observer<ifexm::ObservableEvent>, public common::Observable<Checkpoint>
{
  public:
    explicit MonitorIfDaemon(CheckpointIpcServer& f_ipcServer_r, const char* f_interfaceName_p) noexcept(false)
    {
        if (g_mock_monitor_if_daemon)
        {
            g_mock_monitor_if_daemon->ctor(f_ipcServer_r, f_interfaceName_p);
        }
    }

    IdentifierHash getIdentifier() const noexcept(true) override
    {
        return g_mock_monitor_if_daemon->getIdentifier();
    }

    void updateData(const ifexm::ObservableEvent& f_observable_r) noexcept(true) override
    {
        g_mock_monitor_if_daemon->updateData(f_observable_r);
    }

    void checkForNewData(const std::chrono::nanoseconds f_syncTimestamp) noexcept(true)
    {
        g_mock_monitor_if_daemon->checkForNewData(f_syncTimestamp);
    }
};

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif

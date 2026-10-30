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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_I_WATCHDOG_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_I_WATCHDOG_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/i_watchdog.hpp"

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Reusable gmock mock for IWatchdog, for use by tests of components that service the watchdog.
class MockWatchdog : public IWatchdog
{
  public:
    MOCK_METHOD(bool, init, (), (noexcept, override));
    MOCK_METHOD(void, serviceWatchdog, (), (noexcept, override));
    MOCK_METHOD(void, fireWatchdog, (), (noexcept, override));
    MOCK_METHOD(void, deinit, (), (noexcept, override));
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_I_WATCHDOG_HPP_INCLUDED

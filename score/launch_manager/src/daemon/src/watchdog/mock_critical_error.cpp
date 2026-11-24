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

#include "score/mw/launch_manager/watchdog/mock_critical_error.hpp"

#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal::watchdog
{

std::atomic<MockCriticalError*> MockCriticalError::instance_{nullptr};

MockCriticalError::MockCriticalError() noexcept
{
    MockCriticalError* expected{nullptr};
    if (!instance_.compare_exchange_strong(expected, this))
    {
        ADD_FAILURE() << "Only one MockCriticalError instance may be alive at a time";
    }
}

MockCriticalError::~MockCriticalError() noexcept
{
    MockCriticalError* expected{this};
    static_cast<void>(instance_.compare_exchange_strong(expected, nullptr));
}

MockCriticalError* MockCriticalError::instance() noexcept
{
    return instance_.load();
}

void onCriticalError(CriticalErrorType type, std::string_view msg) noexcept
{
    MockCriticalError* const mock{MockCriticalError::instance()};
    if (mock == nullptr)
    {
        ADD_FAILURE() << "Unexpected onCriticalError(" << toString(type) << "): " << msg;
        return;
    }
    mock->onCriticalError(type, msg);
}

}  // namespace score::mw::lifecycle::internal::watchdog

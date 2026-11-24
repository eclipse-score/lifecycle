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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_CRITICAL_ERROR_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_CRITICAL_ERROR_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/critical_error.hpp"

#include <gmock/gmock.h>

#include <atomic>
#include <string_view>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Reusable gmock mock for onCriticalError(), for use by tests of units that report critical errors.
class MockCriticalError
{
  public:
    MockCriticalError() noexcept;
    ~MockCriticalError() noexcept;

    MockCriticalError(const MockCriticalError&) = delete;
    MockCriticalError(MockCriticalError&&) = delete;
    MockCriticalError& operator=(const MockCriticalError&) = delete;
    MockCriticalError& operator=(MockCriticalError&&) = delete;

    MOCK_METHOD(void, onCriticalError, (CriticalErrorType type, std::string_view msg), (noexcept));

    /// @brief Returns the currently alive instance, or nullptr.
    static MockCriticalError* instance() noexcept;

  private:
    static std::atomic<MockCriticalError*> instance_;
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_MOCK_CRITICAL_ERROR_HPP_INCLUDED

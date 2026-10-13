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

/// @file
/// @brief Link-time substitution infrastructure for mocking ReportRunningImpl
///
/// This file provides alternative implementations of ReportRunningImpl methods that get
/// linked in place of the real implementation during testing. It is NOT a mock itself,
/// but rather the infrastructure that enables GMock to work with code that uses direct
/// object construction (e.g., ReportRunningImpl{}) where dependency injection isn't possible.
///
/// Architecture:
/// 1. ReportRunningImplMock (in .h) - The actual GMock object with MOCK_METHOD declarations
/// 2. Callback functions (GetXxxCallback) - Static storage for routing production calls to mock
/// 3. ReportRunningImpl method implementations (below) - Link-time substitutes that invoke callbacks
///
/// When a test creates a ReportRunningImplMock object, it registers callbacks that route
/// production code calls (e.g., ReportRunningImpl{}.ReportRunningState()) to the GMock object,
/// enabling EXPECT_CALL verification.

#include "score/mw/lifecycle/execution_error.h"
#include "score/mw/lifecycle/lifecycle_client/details/mock_report_running_impl.h"
#include "score/mw/lifecycle/lifecycle_client/details/report_running_impl.hpp"

#include <atomic>
#include <functional>

namespace
{

/// @brief Callback storage for routing ReportRunningImpl constructor calls to the mock
auto& GetConstructorCallback() noexcept
{
    static std::function<void()> constructor_callback{};
    return constructor_callback;
}

/// @brief Callback storage for routing ReportRunningImpl destructor calls to the mock
auto& GetDestructorCallback() noexcept
{
    static std::function<void()> destructor_callback{};
    return destructor_callback;
}

/// @brief Callback storage for routing ReportRunningImpl::ReportRunningState calls to the mock
auto& GetReportRunningStateCallback() noexcept
{
    static std::function<score::Result<std::monostate>()> report_running_state_callback;
    return report_running_state_callback;
}

}  // namespace

namespace score::mw::lifecycle
{

// External access to the static reported flag for test reset
extern std::atomic_bool& GetReportedFlag();

std::atomic_bool& GetReportedFlag()
{
    static std::atomic_bool reported{false};
    return reported;
}

ReportRunningImplMock::ReportRunningImplMock()
{
    GetConstructorCallback() = [this] {
        ctor();
    };
    GetDestructorCallback() = [this] {
        dtor();
    };
    GetReportRunningStateCallback() = [this]() {
        return ReportRunningState();
    };
}

ReportRunningImplMock::~ReportRunningImplMock()
{
    GetConstructorCallback() = nullptr;
    GetDestructorCallback() = nullptr;
    GetReportRunningStateCallback() = nullptr;
}

void ReportRunningImplMock::ResetReportedFlag()
{
    GetReportedFlag() = false;
}

// Link-time substitution implementations
// The following implementations REPLACE the real ReportRunningImpl methods
// when this file is linked into a test. They act as an adapter layer that
// routes calls to the GMock object via the callback functions above.

ReportRunningImpl::ReportRunningImpl() noexcept
{
    auto& constructor_callback = GetConstructorCallback();
    if (constructor_callback)
    {
        constructor_callback();
    }
}

ReportRunningImpl::~ReportRunningImpl() noexcept
{
    auto& destructor_callback = GetDestructorCallback();
    if (destructor_callback)
    {
        destructor_callback();
    }
}

score::Result<std::monostate> ReportRunningImpl::ReportRunningState() const noexcept
{
    auto& callback = GetReportRunningStateCallback();
    if (callback)
    {
        return callback();
    }
    return score::Result<std::monostate>{score::MakeUnexpected(ExecErrc::kCommunicationError)};
}

}  // namespace score::mw::lifecycle

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

#include "tests/utils/mocks/mock_semaphore.hpp"

#include "score/mw/launch_manager/osal/semaphore.hpp"

// NOLINTBEGIN - clang-tidy does not like syscalls

namespace score::mw::lifecycle::internal::osal::mock
{

int g_post_call_count = 0;
bool g_first_post_should_fail = false;
bool g_second_post_should_fail = false;
bool g_timedwait_should_fail = false;

void ResetSemaphoreMockState()
{
    g_post_call_count = 0;
    g_first_post_should_fail = false;
    g_second_post_should_fail = false;
    g_timedwait_should_fail = false;
}

}  // namespace score::mw::lifecycle::internal::osal::mock

// Overrides for score::mw::lifecycle::internal::osal::Semaphore, replacing the real POSIX-backed implementation for
// unit tests that do not link the real semaphore.cpp translation unit.
namespace score::mw::lifecycle::internal::osal
{

OsalReturnType Semaphore::post()
{
    using namespace mock;

    g_post_call_count++;

    // First post is send_sync_.post() in reportKRunningtoDaemon()
    if (g_post_call_count == 1 && g_first_post_should_fail)
    {
        return OsalReturnType::kFail;
    }

    // Second post is the final send_sync_.post() in reportKRunningtoDaemon()
    if (g_post_call_count == 2 && g_second_post_should_fail)
    {
        return OsalReturnType::kFail;
    }

    return OsalReturnType::kSuccess;
}

OsalReturnType Semaphore::timedWait(std::chrono::milliseconds delay)
{
    (void)delay;  // Unused in mock

    if (mock::g_timedwait_should_fail)
    {
        return OsalReturnType::kFail;
    }

    return OsalReturnType::kSuccess;
}

}  // namespace score::mw::lifecycle::internal::osal

// NOLINTEND

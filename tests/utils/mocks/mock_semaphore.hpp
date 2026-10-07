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

#ifndef MOCK_SEMAPHORE
#define MOCK_SEMAPHORE
#include <gmock/gmock.h>

#include <chrono>

#include "score/mw/launch_manager/osal/return_types.hpp"

using OsalReturnType = score::mw::lifecycle::internal::osal::OsalReturnType;

// NOLINTBEGIN - clang-tidy does not like syscalls

class SemaphoreMock
{
  public:
    MOCK_METHOD(OsalReturnType, post, ());
    MOCK_METHOD(OsalReturnType, timedWait, (std::chrono::milliseconds));
};

// NOLINTEND

// Overrides for score::mw::lifecycle::internal::osal::Semaphore::post() / timedWait(), implemented in
// mock_semaphore.cpp. These control variables let tests steer the outcome of individual post()/timedWait()
// invocations without requiring Semaphore to be constructed through an injectable interface.
namespace score::mw::lifecycle::internal::osal::mock
{

// Number of times Semaphore::post() has been invoked since the last ResetSemaphoreMockState() call.
extern int g_post_call_count;

// When true, the first Semaphore::post() call (send_sync_.post()) fails.
extern bool g_first_post_should_fail;

// When true, the second Semaphore::post() call (final send_sync_.post()) fails.
extern bool g_second_post_should_fail;

// When true, Semaphore::timedWait() fails.
extern bool g_timedwait_should_fail;

// Resets all Semaphore mock control state to its default (non-failing) behavior.
void ResetSemaphoreMockState();

}  // namespace score::mw::lifecycle::internal::osal::mock

#endif  // MOCK_SEMAPHORE

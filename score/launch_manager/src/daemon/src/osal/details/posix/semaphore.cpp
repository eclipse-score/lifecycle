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

#include <chrono>
#include <ctime>

#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/os/errno.h"
#include "score/os/semaphore.h"

namespace score::mw::lifecycle::internal::osal
{

namespace
{
constexpr long kNanosecondsPerSecond = 1000000000L;
}  // namespace

OsalReturnType Semaphore::init(uint32_t value, bool shared)
{
    OsalReturnType result = OsalReturnType::kFail;
    const std::int32_t pshared = shared ? 1 : 0;

    if (score::os::Semaphore::instance().sem_init(&sem_, pshared, value).has_value())
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::deinit()
{
    OsalReturnType result = OsalReturnType::kFail;

    // score::os::Semaphore does not provide a wrapper for sem_destroy
    if (sem_destroy(&sem_) == 0)
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::timedWait(std::chrono::milliseconds delay)
{
    // sem_timedwait_monotonic measures the absolute timeout against CLOCK_MONOTONIC,
    // so the wait is not affected by changes to the system clock.
    struct timespec deadline{};
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0)
    {
        return OsalReturnType::kFail;
    }

    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(delay);
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(delay - secs);
    deadline.tv_sec += static_cast<time_t>(secs.count());
    deadline.tv_nsec += static_cast<long>(nsecs.count());
    if (deadline.tv_nsec >= kNanosecondsPerSecond)
    {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= kNanosecondsPerSecond;
    }

    while (true)
    {
        const auto wait_result = score::os::Semaphore::instance().sem_timedwait_monotonic(&sem_, &deadline);
        if (wait_result.has_value())
        {
            return OsalReturnType::kSuccess;
        }

        // The deadline is absolute, so retrying after a signal does not extend the wait.
        if (wait_result.error() == score::os::Error::Code::kOperationWasInterruptedBySignal)
        {
            continue;
        }

        if (wait_result.error() == score::os::Error::Code::kKernelTimeout)
        {
            return OsalReturnType::kTimeout;
        }

        return OsalReturnType::kFail;
    }
}

OsalReturnType Semaphore::post()
{
    OsalReturnType result = OsalReturnType::kFail;

    if (score::os::Semaphore::instance().sem_post(&sem_).has_value())
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::wait()
{
    OsalReturnType result = OsalReturnType::kFail;

    if (score::os::Semaphore::instance().sem_wait(&sem_).has_value())
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

}  // namespace score::mw::lifecycle::internal::osal

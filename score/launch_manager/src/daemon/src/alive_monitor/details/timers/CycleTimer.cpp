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
#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimer.hpp"

#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

namespace score::mw::lifecycle::internal::saf::timers
{
constexpr int CycleTimer::kDeadlineAlreadyOver;

CycleTimer::CycleTimer(const score::mw::lifecycle::internal::saf::timers::OsClockInterface* f_osInterface) noexcept
    : os_interface_{f_osInterface}, sleep_interval_ns_{0}, deadline_{}
{
    static_cast<void>(0U);
}

std::chrono::nanoseconds CycleTimer::init(std::chrono::nanoseconds f_sleepIntervalNs) noexcept
{
    if (nullptr == os_interface_)
    {
        sleep_interval_ns_ = kInitNullInterface;
        return sleep_interval_ns_;
    }

    // check for invalid cycle time
    if (f_sleepIntervalNs.count() <= 0)
    {
        sleep_interval_ns_ = kInitInvalidInterval;
        return sleep_interval_ns_;
    }

    // get an initial time stamp on initialization to check whether the clock is working
    struct timespec tmp = {};
    if (-1 == os_interface_->clockGetTime(&tmp))
    {
        sleep_interval_ns_ = std::chrono::nanoseconds{-1};
    }
    else
    {
        sleep_interval_ns_ = f_sleepIntervalNs;
    }

    return sleep_interval_ns_;
}

std::chrono::nanoseconds CycleTimer::start() noexcept
{
    const int result{os_interface_->clockGetTime(&deadline_)};
    if (0 == result)
    {
        return TimeConversion::convertToNanoSec(deadline_);
    }
    else
    {
        return std::chrono::nanoseconds{0U};
    }
}

struct timespec& CycleTimer::calcNextShot() noexcept(true)
{
    static_assert(sizeof(long) == 8U, "long is not 64 bit");
    // tv_nsec max retval from clockGetTime()   0,000,000,001,000,000,000 ns (1s)
    // tv_nsec absolute max (long)(64bit)       9,223,372,036,854,775,807 ns
    // sleep_interval_ns_ max (std::chrono::nanoseconds) 60,000,000,000 ns (60s)
    // Overflow can occur after 9223372036854775807 / 60000000000 ~ 153722867 cycles
    // which corresponds to 153722867 * 60s = 9223372020s = 153722867min ~ 2562047h ~ 106751d ~ 292y
    deadline_.tv_nsec += sleep_interval_ns_.count();

    handleNanoSecOverflow();

    return deadline_;
}

void CycleTimer::handleNanoSecOverflow() noexcept(true)
{
    constexpr long k_nanoSecondsPerSecond = std::chrono::nanoseconds{std::chrono::seconds{1}}.count();
    while (deadline_.tv_nsec >= k_nanoSecondsPerSecond)
    {
        deadline_.tv_nsec -= k_nanoSecondsPerSecond;
        ++deadline_.tv_sec;
    }
}

}  // namespace score::mw::lifecycle::internal::saf::timers

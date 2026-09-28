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

#ifndef OSCLOCKINTERFACE_HPP_INCLUDED
#define OSCLOCKINTERFACE_HPP_INCLUDED

#include <chrono>

namespace score::mw::lifecycle::internal::saf::timers
{
/// @brief Interface for POSIX clock system calls (free functions such as clock_nanosleep())
/// @note The clockId parameter is set to CLOCK_MONOTONIC for all calls
/// @details The interface allows to mock the direct system calls to 'clock_xx()' for unit
/// and multiple unit tests.
class OsClockInterface
{
  public:
    OsClockInterface() = default;

    virtual ~OsClockInterface() noexcept = default;

    /// @brief No Copy Constructor
    OsClockInterface(const OsClockInterface&) = delete;
    /// @brief No Copy Assignment
    OsClockInterface& operator=(const OsClockInterface&) = delete;
    /// @brief No Move Constructor
    OsClockInterface(OsClockInterface&&) = delete;
    /// @brief No Move Assignment
    OsClockInterface& operator=(OsClockInterface&&) = delete;

    /// @brief Sleep for the amount of time
    /// @details By default the POSIX clock_nanosleep() call shall be used
    virtual int clockNanosleep(int f_flags, const struct timespec* f_req, struct timespec* f_rem) const
    {
        return ::clock_nanosleep(CLOCK_MONOTONIC, f_flags, f_req, f_rem);
    }

    /// @brief Get the current timestamp for the given clock
    /// @details By default the POSIX clock_gettime() call shall be used
    virtual int clockGetTime(struct timespec* f_tp) const
    {
        return ::clock_gettime(CLOCK_MONOTONIC, f_tp);
    }

    /// @brief Get the clock resolution
    /// @details By default the POSIX clock_getres() call shall be used
    virtual int clockGetRes(struct timespec* f_res) const
    {
        return ::clock_getres(CLOCK_MONOTONIC, f_res);
    }

    /// @brief Start the measurement of the initialization process
    /// @param[in] f_osClock_r Clock abstraction
    void startMeasurement()
    {
        // In case getting the timestamp fails, the resulting measurement will be 0
        (void)clockGetTime(&startTime);
    }

    /// @brief Stop the measurement of the initialization process
    /// Prints out the time it took for initialization
    /// @param[in] f_osClock_r Clock abstraction
    long endMeasurement()
    {
        // In case getting the timestamp fails, the resulting measurement will be 0
        (void)clockGetTime(&initFinishedTime);
        const long secDiff{initFinishedTime.tv_sec - startTime.tv_sec};
        const long nsDiff{initFinishedTime.tv_nsec - startTime.tv_nsec};
        // for
        const long ms{(secDiff * 1000 /*ms per sec*/) + (nsDiff / 1000000 /*ns per ms*/)};
        // LM_LOG_DEBUG() << "Alive Monitor initialization took " << ms << " ms";
        return ms;
    }

  private:
    /// @brief Timestamp when main method starts
    struct timespec startTime{0, 0};

    /// @brief Timestamp when initialization finishes (just before kRunning is reported)
    struct timespec initFinishedTime{0, 0};
};

}  // namespace score::mw::lifecycle::internal::saf::timers

#endif

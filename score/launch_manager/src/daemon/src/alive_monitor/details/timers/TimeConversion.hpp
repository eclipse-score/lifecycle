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

#ifndef TIMECONVERSION_HPP_INCLUDED
#define TIMECONVERSION_HPP_INCLUDED

#include <chrono>
#include <ctime>

namespace score::mw::lifecycle::internal::saf::timers
{

/// @brief Convert one unit of time value to another
class TimeConversion
{
  public:
    /// @brief Default Constructor
    TimeConversion() = default;

    /// @brief Copy and Move operations on the class objects are not intended, hence not supported
    TimeConversion(const TimeConversion&) = delete;
    TimeConversion& operator=(const TimeConversion&) = delete;
    TimeConversion(TimeConversion&&) = delete;
    TimeConversion& operator=(TimeConversion&&) = delete;

    /// @brief Default Destructor
    virtual ~TimeConversion() = default;

    /// @brief Convert time value in timespec (second and nanosecond) to nanoseconds
    /// @param [in] f_timespec    Time value in timespec (second and nanosecond)
    /// @return std::chrono::nanoseconds    Time value converted to nanoseconds
    ///                           (returns 0 in case of an invalid timespec)
    static std::chrono::nanoseconds convertToNanoSec(const timespec f_timespec) noexcept(true);

    /// @brief Convert time value in milliseconds to nanoseconds
    /// @param [in]    f_timeValueMilliSec    Time value in milliseconds unit
    /// @return std::chrono::nanoseconds  Time value converted to nanoseconds
    ///                         (returns 0 in case of an error)
    static std::chrono::nanoseconds convertMilliSecToNanoSec(
        const std::chrono::milliseconds f_timeValueMilliSec) noexcept(true);

    /// @brief Convert time value in nanoseconds to milliseconds
    /// @param [in]    f_timeValueNanoSec    Time value in nanoseconds unit
    /// @return double  Time value converted to milliseconds
    static std::chrono::milliseconds convertNanoSecToMilliSec(
        const std::chrono::nanoseconds f_timeValueNanoSec) noexcept(true);
};

}  // namespace score::mw::lifecycle::internal::saf::timers

#endif

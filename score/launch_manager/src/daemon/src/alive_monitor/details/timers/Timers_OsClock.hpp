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

#ifndef TIMERS_OSCLOCK_HPP_INCLUDED
#define TIMERS_OSCLOCK_HPP_INCLUDED

#include <chrono>
namespace score::mw::lifecycle::internal::saf::timers
{

/// @brief Operating system clock interface
/// @details The OsClock class provides methods to interact with the operating system
/// specific clock interfaces.
class OsClock
{
  public:
    /// @brief Constructor
    OsClock() = default;
    /// @brief Copy and Move operations on the class objects are
    /// not allowed!
    OsClock(const OsClock&) = delete;
    OsClock& operator=(const OsClock&) = delete;
    OsClock(OsClock&&) = delete;
    OsClock& operator=(OsClock&&) = delete;
    /// @brief Destructor
    virtual ~OsClock() = default;

    /// @brief Get monotonic increasing system Clock
    /// @return System clock in nanoseconds or error indicator.
    /// @retval 0 in case of an error.
    static std::chrono::nanoseconds getMonotonicSystemClock(void) noexcept(true);
};

}  // namespace score::mw::lifecycle::internal::saf::timers

#endif

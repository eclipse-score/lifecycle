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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_CRITICAL_ERROR_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_CRITICAL_ERROR_HPP_INCLUDED

#include <cstdint>
#include <string_view>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Type of a critical error of the Launch Manager.
enum class CriticalErrorType : std::uint8_t
{
    kGeneralError,           ///< Unrecoverable error without a more specific type.
    kQueueOverflow,          ///< An internal event queue overflowed; events were lost.
    kThreadAliveCheckFailed  ///< A thread alive check failed, it may be indicative of a deadlock.
};

/// @brief Returns the name of the error type, for logging.
/// @param type The error type.
/// @return The name of the error type, or "Unknown" for out-of-range values.
constexpr std::string_view toString(CriticalErrorType type) noexcept
{
    switch (type)
    {
        case CriticalErrorType::kGeneralError:
            return "GeneralError";
        case CriticalErrorType::kQueueOverflow:
            return "QueueOverflow";
        case CriticalErrorType::kThreadAliveCheckFailed:
            return "ThreadAliveCheckFailed";
        default:
            return "Unknown";
    }
}

/// @brief Called in case of a critical error detected in the Launch Manager.
/// @details The implementation triggers the system reaction, e.g. a reset or
/// terminating the Launch Manager (the default implementation logs the error and calls std::abort()).
///
/// Contract:
/// - Production implementations must not return.
/// - May be called from any thread, possibly concurrently with IWatchdog::serviceWatchdog() on the main thread.
/// - Must not be called in a forked child process before exec.
///
/// @param type Type of the critical error.
/// @param msg Human readable description of the error.
void onCriticalError(CriticalErrorType type, std::string_view msg) noexcept;

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_CRITICAL_ERROR_HPP_INCLUDED

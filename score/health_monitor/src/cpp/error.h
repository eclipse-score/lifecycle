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

#ifndef SCORE_HEALTH_MONITOR_ERROR_H
#define SCORE_HEALTH_MONITOR_ERROR_H

#include "score/result/error.h"

#include <string_view>

namespace score::health_monitor
{

/// @brief Error codes of the health monitor API.
enum class ErrorCode : score::result::ErrorCode
{
    /// @brief No monitor or deadline has this tag.
    kUnknownTag = 1,

    /// @brief The monitor or deadline with this tag was already returned once.
    kAlreadyTaken = 2,

    /// @brief `Start()` found a monitor that the application did not take.
    kNotTaken = 3,

    /// @brief The health monitor runs, so monitors can no longer be taken or started.
    kAlreadyStarted = 4,

    /// @brief The builder has no monitor.
    kNoMonitors = 5,

    /// @brief An argument or a configuration value is not valid.
    kInvalidArgument = 6,

    /// @brief The deadline already has a handle.
    kDeadlineRunning = 7,

    /// @brief The deadline missed its range earlier, so it cannot start again.
    kDeadlineFailed = 8,

    /// @brief The current state does not allow this transition.
    kInvalidTransition = 9,

    /// @brief An earlier invalid transition put the logic monitor in a failed state.
    kInvalidState = 10,

    /// @brief The health monitor cannot connect to its supervisor.
    kSupervisorUnavailable = 11,
};

/// @brief Translates `ErrorCode` values to messages.
class ErrorDomain final : public score::result::ErrorDomain
{
  public:
    /// @brief Returns the message of `code`, or "Unknown error" for a value that is not an `ErrorCode`.
    std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override;
};

/// @brief Creates an error in the health monitor error domain.
/// @param user_message Extra text for the error. The error keeps a view, so the text must outlive the error.
/// @note Argument-dependent lookup finds this function, so `score::MakeUnexpected(ErrorCode::kUnknownTag)` works.
score::result::Error MakeError(const ErrorCode code, const std::string_view user_message = "") noexcept;

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_ERROR_H

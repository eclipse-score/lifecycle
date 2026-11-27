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

#include "score/health_monitor/error.h"

namespace score::health_monitor
{

namespace
{
const ErrorDomain kErrorDomain{};
}  // namespace

std::string_view ErrorDomain::MessageFor(const score::result::ErrorCode& code) const noexcept
{
    switch (static_cast<ErrorCode>(code))
    {
        case ErrorCode::kUnknownTag:
            return "Unknown tag";
        case ErrorCode::kAlreadyTaken:
            return "Already taken";
        case ErrorCode::kNotTaken:
            return "Monitor not taken";
        case ErrorCode::kAlreadyStarted:
            return "Health monitor already started";
        case ErrorCode::kNoMonitors:
            return "No monitors";
        case ErrorCode::kInvalidArgument:
            return "Invalid argument";
        case ErrorCode::kDeadlineRunning:
            return "Deadline already running";
        case ErrorCode::kDeadlineFailed:
            return "Deadline failed";
        case ErrorCode::kInvalidTransition:
            return "Invalid transition";
        case ErrorCode::kInvalidState:
            return "Invalid state";
        case ErrorCode::kSupervisorUnavailable:
            return "Supervisor unavailable";
        default:
            return "Unknown error";
    }
}

score::result::Error MakeError(const ErrorCode code, const std::string_view user_message) noexcept
{
    return {static_cast<score::result::ErrorCode>(code), kErrorDomain, user_message};
}

}  // namespace score::health_monitor

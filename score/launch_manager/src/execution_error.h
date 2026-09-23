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

/// @file

#ifndef SCORE_LCM_ERROR_DOMAIN_H_
#define SCORE_LCM_ERROR_DOMAIN_H_

#include "score/result/result.h"

namespace score::mw::lifecycle
{

enum class ExecErrc : score::result::ErrorCode
{
    kGeneralError = 1,        ///< Some unspecified error occurred
    kInvalidArguments = 2,    ///< Invalid argument was passed
    kCommunicationError = 3,  ///< Communication error occurred
    kMetaModelError = 4,      ///< Wrong meta model identifier passed to a function
    kCancelled = 5,           ///< Transition to the requested run target was cancelled by a newer request
    kFailed = 6,              ///< Requested operation could not be performed
    kFailedUnexpectedTerminationOnExit = 7,   ///< Unexpected termination while stopping the previous run target
    kFailedUnexpectedTerminationOnEnter = 8,  ///< Unexpected termination while starting the next run target
    kInvalidTransition = 9,         ///< Transition invalid (e.g. report kRunning when already in Running Process State)
    kAlreadyInState = 10,           ///< Transition to the requested run target failed because it is already active
    kInTransitionToSameState = 11,  ///< Transition to the requested run target failed because the same transition
                                    ///< is already in progress
    kActivationInProgress =
        12,  ///< A run target activation is already in progress; no single run target is currently active
    kRequestQueueIsFull = 13,    ///< The activation request queue is full; the request was discarded
    kRunTargetDoesntExist = 14,  ///< The requested run target does not exist in the current configuration
    kNotImplemented = 15,        ///< The requested functionality is not yet implemented
};

class ExecErrorDomain final : public score::result::ErrorDomain
{

    [[nodiscard]] std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<ExecErrc>(code))
        {
            case ExecErrc::kGeneralError:
                return "Some unspecified error occurred";
            case ExecErrc::kInvalidArguments:
                return "An invalid argument was passed";
            case ExecErrc::kCommunicationError:
                return "A communication error occurred";
            case ExecErrc::kMetaModelError:
                return "Wrong meta model identifier passed to a function";
            case ExecErrc::kCancelled:
                return "Transition to the requested run target was cancelled by a newer request";
            case ExecErrc::kFailed:
                return "Requested operation could not be performed";
            case ExecErrc::kFailedUnexpectedTerminationOnExit:
                return "Unexpected termination while stopping the previous run target";
            case ExecErrc::kFailedUnexpectedTerminationOnEnter:
                return "Unexpected termination while starting the next run target";
            case ExecErrc::kInvalidTransition:
                return "Transition invalid (e.g. report kRunning when already in Running Process State)";
            case ExecErrc::kAlreadyInState:
                return "Transition to the requested run target failed because it is already active";
            case ExecErrc::kInTransitionToSameState:
                return " Transition to the requested run target failed because the same transition"
                       " is already in progress";
            case ExecErrc::kActivationInProgress:
                return "A run target activation is already in progress; no single run target is currently active";
            case ExecErrc::kRequestQueueIsFull:
                return "The activation request queue is full; the request was discarded";
            case ExecErrc::kRunTargetDoesntExist:
                return "The requested run target does not exist in the current configuration";
            case ExecErrc::kNotImplemented:
                return "The requested functionality is not yet implemented";
            default:
                return "Unknown error";
        }
    }
};

/// @brief The single ExecErrorDomain instance every ExecErrc-based Error refers to.
inline constexpr ExecErrorDomain g_ExecErrorDomain{};

constexpr score::result::Error MakeError(ExecErrc code, const std::string_view user_message = "") noexcept
{
    return score::result::Error{static_cast<score::result::ErrorCode>(code), g_ExecErrorDomain, user_message};
}

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_ERROR_DOMAIN_H_

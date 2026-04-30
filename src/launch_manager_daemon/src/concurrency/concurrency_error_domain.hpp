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

#ifndef CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED
#define CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED

#include "score/result/result.h"

#include <string_view>

namespace score::lcm::internal
{

enum class ConcurrencyErrc : score::result::ErrorCode
{
    /// @brief An OS call returned an error.
    kOsError = 1,

    // @brief The container has overflowed.
    kOverflow = 2,

    // @brief The container has stopped.
    kStopped = 3,

    // @brief A timeout was triggered.
    kTimeout = 4,
};

class ConcurrencyErrorDomain final : public score::result::ErrorDomain
{
    [[nodiscard]] std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<ConcurrencyErrc>(code))
        {
            case ConcurrencyErrc::kOsError:
                return "An OS call returned an error";

            case ConcurrencyErrc::kOverflow:
                return "The container has overflowed";

            case ConcurrencyErrc::kStopped:
                return "The container has been stopped";

            case ConcurrencyErrc::kTimeout:
                return "A timer was triggered";

            default:
                return "Unknown concurrency error";
        }
    }
};

constexpr ConcurrencyErrorDomain g_ConcurrencyErrorDomain{};

constexpr score::result::Error MakeError(ConcurrencyErrc code, const std::string_view user_message = "") noexcept
{
    return score::result::Error{static_cast<score::result::ErrorCode>(code), g_ConcurrencyErrorDomain, user_message};
}

}  // namespace score::lcm::internal

#endif  // CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED

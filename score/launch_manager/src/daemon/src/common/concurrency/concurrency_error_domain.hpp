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

#include <cstdint>
#include <ostream>
#include <string_view>

namespace score::lcm::internal
{

/// @brief Error codes must use score::result::ErrorCode as their underlying type so that they can be
///        wrapped into a score::result::Error (see MakeError() below and ConcurrencyErrorDomain::MessageFor()).
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

inline std::ostream& operator<<(std::ostream& os, ConcurrencyErrc errc) noexcept
{
    switch (errc)
    {
        case ConcurrencyErrc::kOsError:  return os << "kOsError";
        case ConcurrencyErrc::kOverflow: return os << "kOverflow";
        case ConcurrencyErrc::kStopped:  return os << "kStopped";
        case ConcurrencyErrc::kTimeout:  return os << "kTimeout";
        default:                         return os << static_cast<score::result::ErrorCode>(errc);
    }
}

/// @brief Error domain translating ConcurrencyErrc values into human readable messages, as required by
///        score::Result / score::result::Error.
class ConcurrencyErrorDomain final : public score::result::ErrorDomain
{
  public:
    [[nodiscard]] std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<ConcurrencyErrc>(code))
        {
            case ConcurrencyErrc::kOsError:
                return "An OS call returned an error";
            case ConcurrencyErrc::kOverflow:
                return "The container has overflowed";
            case ConcurrencyErrc::kStopped:
                return "The container has stopped";
            case ConcurrencyErrc::kTimeout:
                return "A timeout was triggered";
            default:
                return "Unknown error";
        }
    }
};

constexpr ConcurrencyErrorDomain g_ConcurrencyErrorDomain{};

/// @brief Constructs a score::result::Error from a ConcurrencyErrc. Found via ADL from score::result::Error's
///        constructor and from score::MakeUnexpected().
constexpr score::result::Error MakeError(ConcurrencyErrc code, const std::string_view user_message = "") noexcept
{
    return score::result::Error{static_cast<score::result::ErrorCode>(code), g_ConcurrencyErrorDomain, user_message};
}

}  // namespace score::lcm::internal

#ifdef LC_LOG_SCORE_MW_LOG
#include "score/mw/log/logger.h"

namespace score::lcm::internal
{

inline score::mw::log::LogStream& operator<<(score::mw::log::LogStream& os, ConcurrencyErrc errc) noexcept
{
    switch (errc)
    {
        case ConcurrencyErrc::kOsError:
            return os << "kOsError";
        case ConcurrencyErrc::kOverflow:
            return os << "kOverflow";
        case ConcurrencyErrc::kStopped:
            return os << "kStopped";
        case ConcurrencyErrc::kTimeout:
            return os << "kTimeout";
        default:
            return os << static_cast<std::uint8_t>(errc);
    }
}

} // score::lcm::internal

#endif  // LC_LOG_SCORE_MW_LOG

#endif  // CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED

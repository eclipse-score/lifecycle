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

#ifndef SCORE_HEALTH_MONITOR_TAG_H
#define SCORE_HEALTH_MONITOR_TAG_H

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace score::health_monitor
{
namespace details
{

/// @brief Name of a monitor, deadline or state, compared by text.
/// @tparam Kind Empty type that makes each tag kind a distinct type.
/// @warning The tag keeps a pointer to the text and does not copy it.
/// A tag made from a local `char` array dangles when the array goes out of scope.
template <typename Kind>
class Tag final
{
  public:
    /// @brief Creates a tag that refers to `text`.
    /// @pre `text` has static storage duration, which is always true for a string literal.
    template <std::size_t N>
    constexpr explicit Tag(const char (&text)[N]) noexcept : text_{text, N - 1U}
    {
    }

    /// @brief Rejects a mutable buffer: it is not a string literal, and its size is not the text length.
    template <std::size_t N>
    Tag(char (&text)[N]) = delete;

    /// @brief Returns the text. It ends with a null character that `Size()` does not count.
    constexpr const char* Data() const noexcept
    {
        return text_.data();
    }

    /// @brief Returns the length of the text without the terminating null character.
    constexpr std::size_t Size() const noexcept
    {
        return text_.size();
    }

    /// @brief Compares the text, not the pointer.
    constexpr bool operator==(const Tag& other) const noexcept
    {
        return text_ == other.text_;
    }

    /// @brief Returns true if the texts differ.
    constexpr bool operator!=(const Tag& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    /// @brief Text without the terminating null character. The tag does not own it.
    std::string_view text_;
};

/// @brief Kind of `MonitorTag`.
struct MonitorTagKind
{
};
/// @brief Kind of `DeadlineTag`.
struct DeadlineTagKind
{
};
/// @brief Kind of `StateTag`.
struct StateTagKind
{
};

}  // namespace details

/// @brief Names a monitor in the health monitor.
using MonitorTag = details::Tag<details::MonitorTagKind>;
/// @brief Names a deadline in a deadline monitor.
using DeadlineTag = details::Tag<details::DeadlineTagKind>;
/// @brief Names a state in a logic monitor.
using StateTag = details::Tag<details::StateTagKind>;

static_assert(
    std::is_standard_layout_v<MonitorTag> && std::is_trivially_copyable_v<MonitorTag>,
    "A C interface passes tags by value.");
static_assert(
    std::is_standard_layout_v<DeadlineTag> && std::is_trivially_copyable_v<DeadlineTag>,
    "A C interface passes tags by value.");
static_assert(
    std::is_standard_layout_v<StateTag> && std::is_trivially_copyable_v<StateTag>,
    "A C interface passes tags by value.");

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_TAG_H

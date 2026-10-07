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
#ifndef SCORE_HM_COMMON_H
#define SCORE_HM_COMMON_H

#include <score/assert.hpp>
#include <chrono>
#include <optional>

namespace score::mw::health
{

/// FFI internal helpers
namespace internal
{

/// Internal success representation.
constexpr int kSuccess = 0;

/// Internal return code.
using FFICode = uint8_t;

/// Opaque handle type for Rust managed object
using FFIHandle = void*;

/// Droppable wrapper that denotes that the object can be dropped by Rust side
template <typename T>
class RustDroppable
{
  public:
    virtual ~RustDroppable() = default;

  protected:
    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping
    std::optional<FFIHandle> DropByRust()
    {
        return static_cast<T*>(this)->DropByRustImpl();
    }
};

/// Wrapper for FFIHandle that ensures proper dropping via provided drop function
class DroppableFFIHandle
{
  public:
    using DropFn = internal::FFICode (*)(FFIHandle);

    DroppableFFIHandle(FFIHandle handle, DropFn drop_fn);

    DroppableFFIHandle(const DroppableFFIHandle&) = delete;
    DroppableFFIHandle& operator=(const DroppableFFIHandle&) = delete;

    DroppableFFIHandle(DroppableFFIHandle&& other) noexcept;
    DroppableFFIHandle& operator=(DroppableFFIHandle&& other) noexcept;

    /// Get the underlying FFI handle if it was not dropped before
    std::optional<FFIHandle> AsRustHandle() const;

    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping
    std::optional<FFIHandle> DropByRust();

    virtual ~DroppableFFIHandle();

  private:
    FFIHandle handle_;
    DropFn drop_fn_;
};

}  // namespace internal

enum class Error : internal::FFICode
{
    NullParameter = internal::kSuccess + 1,
    NotFound,
    AlreadyExists,
    InvalidArgument,
    WrongState,
    Failed
};

///
/// Time range representation with minimum and maximum durations in milliseconds.
///
class TimeRange
{
  public:
    TimeRange(std::chrono::milliseconds min_ms, std::chrono::milliseconds max_ms) : min_ms_(min_ms), max_ms_(max_ms)
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(min_ms_ <= max_ms_);
    }

    uint32_t MinMs() const
    {
        return min_ms_.count();
    }

    uint32_t MaxMs() const
    {
        return max_ms_.count();
    }

    /// @deprecated Use `MinMs()` instead. Removed in the release after v0.10.
    [[deprecated("Use MinMs() instead. The snake_case API is removed in the release after v0.10.")]] uint32_t min_ms()
        const
    {
        return MinMs();
    }

    /// @deprecated Use `MaxMs()` instead. Removed in the release after v0.10.
    [[deprecated("Use MaxMs() instead. The snake_case API is removed in the release after v0.10.")]] uint32_t max_ms()
        const
    {
        return MaxMs();
    }

  private:
    const std::chrono::milliseconds min_ms_;
    const std::chrono::milliseconds max_ms_;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_COMMON_H

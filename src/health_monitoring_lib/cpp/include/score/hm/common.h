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

#include <cassert>
#include <chrono>
#include <cstdint>
#include <optional>

namespace score::hm
{

/// FFI internal helpers
namespace internal
{

/// Internal success representation.
constexpr int kSuccess = 0;

/// Internal return code.
using FFICode = uint8_t;

/// Abort if provided code is not equal to `kSuccess`.
void abort_on_error(FFICode code);

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
    std::optional<FFIHandle> drop_by_rust()
    {
        return static_cast<T*>(this)->__drop_by_rust_impl();
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
    std::optional<FFIHandle> as_rust_handle() const;

    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping
    std::optional<FFIHandle> drop_by_rust();

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
/// Identifier tag used to uniquely identify entities within the health monitoring system.
///
class IdentTag
{
  public:
    /// Create a new IdentTag from a C-style string.
    template <size_t N>
    explicit IdentTag(const char (&tag)[N]) : tag_(tag), len_(N - 1)
    {
    }

  private:
    /// SAFETY: This has to be FFI compatible with the Rust side representation.
    const char* const tag_;
    size_t len_;
};

///
/// Time range representation with minimum and maximum durations in milliseconds.
///
class TimeRange
{
  public:
    TimeRange(std::chrono::milliseconds min_ms, std::chrono::milliseconds max_ms) : min_ms_(min_ms), max_ms_(max_ms)
    {
        assert(min_ms_ <= max_ms_);
    }

    const uint32_t min_ms() const
    {
        return min_ms_.count();
    }

    const uint32_t max_ms() const
    {
        return max_ms_.count();
    }

  private:
    const std::chrono::milliseconds min_ms_;
    const std::chrono::milliseconds max_ms_;
};

}  // namespace score::hm

#endif  // SCORE_HM_COMMON_H

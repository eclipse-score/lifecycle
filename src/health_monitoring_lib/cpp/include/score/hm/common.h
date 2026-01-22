#ifndef SCORE_HM_COMMON_H
#define SCORE_HM_COMMON_H

#include <score/optional.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
namespace score::hm
{

constexpr int kSuccess = 0;

enum class Error
{
    NotFound = kSuccess + 1,
    AlreadyExists = kSuccess + 2,
    InvalidArgument = kSuccess + 3,
    WrongState = kSuccess + 4,
    Failed = kSuccess + 5
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
    TimeRange(std::chrono::milliseconds min_ms, std::chrono::milliseconds max_ms) : min_ms(min_ms), max_ms(max_ms) {}

    const uint32_t min_as_u32() const
    {
        return min_ms.count();
    }

    const uint32_t max_as_u32() const
    {
        return max_ms.count();
    }

  private:
    const std::chrono::milliseconds min_ms;
    const std::chrono::milliseconds max_ms;
};

/// FFI internal helpers
namespace internal
{

/// Opaque handle type for Rust managed object
using FFIHandle = void*;

/// Droppable wrapper that denotes that the object can be dropped by Rust side
template <typename T>
class RustDroppable
{
  public:
    ~RustDroppable() = default;

    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping
    ::score::cpp::optional<FFIHandle> drop_by_rust()
    {
        return static_cast<T*>(this)->__drop_by_rust_impl();
    }
};

/// Wrapper for FFIHandle that ensures proper dropping via provided drop function
class DroppableFFIHandle
{
  public:
    using DropFn = void (*)(FFIHandle);

    DroppableFFIHandle(FFIHandle handle, DropFn drop_fn);

    DroppableFFIHandle(const DroppableFFIHandle&) = delete;
    DroppableFFIHandle& operator=(const DroppableFFIHandle&) = delete;

    DroppableFFIHandle(DroppableFFIHandle&& other) noexcept;
    DroppableFFIHandle& operator=(DroppableFFIHandle&& other) noexcept;

    /// Get the underlying FFI handle if it was not dropped before
    ::score::cpp::optional<FFIHandle> as_rust_handle() const;

    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping
    ::score::cpp::optional<FFIHandle> drop_by_rust();

    ~DroppableFFIHandle();

  private:
    FFIHandle handle_;
    DropFn drop_fn_;
};

}  // namespace internal

}  // namespace score::hm

#endif  // SCORE_HM_COMMON_H

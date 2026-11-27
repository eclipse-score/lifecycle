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

#ifndef SCORE_HEALTH_MONITOR_TAKEN_H
#define SCORE_HEALTH_MONITOR_TAKEN_H

#include <score/assert.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace score::health_monitor
{

/// @brief Move-only access to a monitor or a deadline that a `Get*` call returned.
/// @tparam T The interface of the taken object.
/// @note The handle shares ownership of the object with the health monitor.
/// The object stays valid after the health monitor is destroyed.
/// A move leaves the source empty, and access through an empty handle aborts.
template <typename T>
class Taken final
{
  public:
    /// @brief Takes a share of `object`.
    /// @pre `object` is not `nullptr`.
    explicit Taken(std::shared_ptr<T> object) noexcept : object_{std::move(object)}
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD(object_ != nullptr);
    }

    Taken(const Taken&) = delete;

    /// @brief Takes over the object of `other` and leaves `other` empty.
    Taken(Taken&& other) noexcept = default;

    /// @brief Takes over the object of a handle to a derived type and leaves `other` empty.
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Taken(Taken<U>&& other) noexcept : object_{std::move(other.object_)}
    {
    }

    ~Taken() = default;

    Taken& operator=(const Taken&) = delete;

    /// @brief Takes over the object of `other` and leaves `other` empty.
    Taken& operator=(Taken&& other) noexcept = default;

    /// @brief Returns the taken object.
    /// @pre The handle is not empty.
    T& Get() const noexcept
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD(object_ != nullptr);
        return *object_;
    }

    /// @copydoc Get
    T& operator*() const noexcept
    {
        return Get();
    }

    /// @copydoc Get
    T* operator->() const noexcept
    {
        return &Get();
    }

  private:
    template <typename U>
    friend class Taken;

    /// @brief Share of the taken object, or `nullptr` after a move.
    std::shared_ptr<T> object_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_TAKEN_H

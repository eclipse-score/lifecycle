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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_FETCH_UPDATE_H
#define SCORE_HEALTH_MONITOR_INTERNAL_FETCH_UPDATE_H

#include <score/expected.hpp>

#include <atomic>
#include <optional>

namespace score::health_monitor::details
{

/// @brief Replaces the value of `atomic` with `update(value)` in a compare-and-swap loop.
/// @param set_order Memory order of a successful store.
/// @param fetch_order Memory order of each read, including the read by a failed compare-and-swap.
/// @return The value before the store, or as error the last value for which `update` returns no value.
/// @warning `update` can run more than once, so it must not change any state other than its result.
template <typename T, typename Function>
score::cpp::expected<T, T> FetchUpdate(
    std::atomic<T>& atomic,
    const std::memory_order set_order,
    const std::memory_order fetch_order,
    Function update) noexcept
{
    T previous{atomic.load(fetch_order)};
    while (const std::optional<T> next{update(previous)})
    {
        // A failed compare-and-swap writes the value it found into `previous`, so the next `update` sees it.
        if (atomic.compare_exchange_weak(previous, *next, set_order, fetch_order))
        {
            return previous;
        }
    }

    return score::cpp::make_unexpected(previous);
}

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_FETCH_UPDATE_H

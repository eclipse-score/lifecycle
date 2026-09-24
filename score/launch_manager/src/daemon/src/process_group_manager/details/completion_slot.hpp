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

#ifndef SCORE_LCM_COMPLETION_SLOT_HPP_INCLUDED
#define SCORE_LCM_COMPLETION_SLOT_HPP_INCLUDED

#include "score/concurrency/notification.h"
#include "score/stop_token.hpp"

#include <optional>
#include <utility>

namespace score::mw::lifecycle::internal
{

/// @brief A non-allocating, single-producer, single-consumer one-shot completion slot.
///
/// Designed to be allocated on the caller's stack frame. The caller passes a pointer
/// to the slot via an event to the main event loop thread and waits for the result.
/// This completely avoids dynamic memory allocation in contrast to std::promise/future
/// or InterruptiblePromise/Future.
template <typename T>
class CompletionSlot final
{
  public:
    CompletionSlot() = default;
    ~CompletionSlot() = default;
    CompletionSlot(const CompletionSlot&) = delete;
    CompletionSlot& operator=(const CompletionSlot&) = delete;
    CompletionSlot(CompletionSlot&&) = delete;
    CompletionSlot& operator=(CompletionSlot&&) = delete;

    /// @brief Publishes the result and wakes the waiting thread. Called once, typically by the main thread.
    void complete(T value)
    {
        value_ = std::move(value);
        done_.notify();
    }

    /// @brief Blocks until complete() has run or interruption is requested via token.
    /// @param token A stop_token that can abort the wait.
    /// @return The completed value, or std::nullopt if the wait was aborted.
    [[nodiscard]] std::optional<T> wait(score::cpp::stop_token token = {})
    {
        if (!done_.waitWithAbort(token))
        {
            return std::nullopt;
        }
        return std::move(value_);
    }

  private:
    /// @brief Inline storage for the result; no dynamic memory allocation.
    std::optional<T> value_{};

    /// @brief Notification held by value (mutex + cv + flag). No dynamic memory allocation.
    score::concurrency::Notification done_{};
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPLETION_SLOT_HPP_INCLUDED

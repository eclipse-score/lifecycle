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

#ifndef SCORE_LCM_PENDING_ACTIVATION_QUEUE
#define SCORE_LCM_PENDING_ACTIVATION_QUEUE

#include "score/containers/non_relocatable_vector.h"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/result/result.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace score::mw::lifecycle::internal
{

/// @brief Fixed-capacity FIFO queue of pending Run Target activations.
/// @details All storage is allocated once, in `Create()`. The storage is a
///          `NonRelocatableVector` that is filled to capacity up front and
///          used as a ring buffer, so no allocation happens afterwards.
///          Not thread-safe: it must only be used from a single thread.
class PendingActivationQueue final
{
  public:
    explicit PendingActivationQueue(std::uint8_t capacity);
    PendingActivationQueue(const PendingActivationQueue&) = delete;
    PendingActivationQueue& operator=(const PendingActivationQueue&) = delete;
    PendingActivationQueue(PendingActivationQueue&&) noexcept = default;
    PendingActivationQueue& operator=(PendingActivationQueue&&) noexcept = default;
    ~PendingActivationQueue() = default;

    /// @brief Append an entry to the back of the queue.
    /// @return false (and leave the queue unchanged) if the queue is full.
    [[nodiscard]] bool push(IdentifierHash id) noexcept;

    /// @brief Remove and return the entry at the front of the queue.
    /// @return std::nullopt if the queue is empty.
    [[nodiscard]] std::optional<IdentifierHash> pop() noexcept;

    /// @brief The most recently pushed entry that is still queued.
    /// @return std::nullopt if the queue is empty.
    [[nodiscard]] std::optional<IdentifierHash> back() const noexcept;

    /// @brief Remove all entries.
    void clear() noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool full() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::uint8_t capacity() const noexcept;

  private:
    /// @brief Index into `storage_` of the n-th element counted from the front.
    [[nodiscard]] std::size_t indexOf(std::size_t offset) const noexcept;

    std::uint8_t capacity_;
    std::size_t head_{0U};
    std::size_t size_{0U};
    score::containers::NonRelocatableVector<IdentifierHash> storage_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PENDING_ACTIVATION_QUEUE

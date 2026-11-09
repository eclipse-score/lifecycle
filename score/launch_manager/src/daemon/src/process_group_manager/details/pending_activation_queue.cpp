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

#include "score/mw/launch_manager/process_group_manager/details/pending_activation_queue.hpp"

#include "score/mw/lifecycle/execution_error.h"

namespace score::mw::lifecycle::internal
{

PendingActivationQueue::PendingActivationQueue(const std::uint8_t capacity)
    : capacity_(capacity), storage_(static_cast<std::size_t>(capacity))
{
    // Fill all slots now so that no allocation is needed later.
    for (std::size_t i = 0U; i < static_cast<std::size_t>(capacity); ++i)
    {
        static_cast<void>(storage_.emplace_back());
    }
}

std::size_t PendingActivationQueue::indexOf(const std::size_t offset) const noexcept
{
    return (head_ + offset) % static_cast<std::size_t>(capacity_);
}

bool PendingActivationQueue::push(const IdentifierHash id) noexcept
{
    if (full())
    {
        return false;
    }

    storage_[indexOf(size_)] = id;
    ++size_;
    return true;
}

std::optional<IdentifierHash> PendingActivationQueue::pop() noexcept
{
    if (empty())
    {
        return std::nullopt;
    }

    const IdentifierHash front = storage_[head_];
    head_ = indexOf(1U);
    --size_;
    return front;
}

std::optional<IdentifierHash> PendingActivationQueue::back() const noexcept
{
    if (empty())
    {
        return std::nullopt;
    }

    return storage_[indexOf(size_ - 1U)];
}

void PendingActivationQueue::clear() noexcept
{
    head_ = 0U;
    size_ = 0U;
}

bool PendingActivationQueue::empty() const noexcept
{
    return size_ == 0U;
}

bool PendingActivationQueue::full() const noexcept
{
    return size_ == static_cast<std::size_t>(capacity_);
}

std::size_t PendingActivationQueue::size() const noexcept
{
    return size_;
}

std::uint8_t PendingActivationQueue::capacity() const noexcept
{
    return capacity_;
}

}  // namespace score::mw::lifecycle::internal

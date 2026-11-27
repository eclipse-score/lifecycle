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

#include "score/health_monitor/deadline.h"

#include "score/health_monitor/error.h"

namespace score::health_monitor
{

score::Result<DeadlineHandle> Deadline::Start() noexcept
{
    // Acquire pairs with the release in DeadlineHandle::Stop(), so DoStart() sees the state DoStop() wrote.
    if (running_.load(std::memory_order_acquire))
    {
        return score::MakeUnexpected(ErrorCode::kDeadlineRunning);
    }

    const auto started = DoStart();
    if (!started.has_value())
    {
        return score::MakeUnexpected<DeadlineHandle>(started.error());
    }

    running_.store(true, std::memory_order_relaxed);
    return DeadlineHandle{*this};
}

DeadlineHandle::DeadlineHandle(Deadline& deadline) noexcept : deadline_{&deadline}
{
}

DeadlineHandle::DeadlineHandle(DeadlineHandle&& other) noexcept : deadline_{other.deadline_}
{
    other.deadline_ = nullptr;
}

DeadlineHandle::~DeadlineHandle()
{
    Stop();
}

void DeadlineHandle::Stop() noexcept
{
    if (deadline_ == nullptr)
    {
        return;
    }

    deadline_->DoStop();
    deadline_->running_.store(false, std::memory_order_release);
    deadline_ = nullptr;
}

}  // namespace score::health_monitor

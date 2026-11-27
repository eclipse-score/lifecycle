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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_H
#define SCORE_HEALTH_MONITOR_DEADLINE_H

#include "score/result/result.h"

#include <atomic>

namespace score::health_monitor
{

class DeadlineHandle;

/// @brief One deadline of a deadline monitor: the time from `Start()` to `DeadlineHandle::Stop()` must be in its range.
/// @note The class is not thread-safe.
/// @note A missed deadline stays failed: every later `Start()` returns `ErrorCode::kDeadlineFailed`.
class Deadline
{
  public:
    Deadline(const Deadline&) = delete;
    Deadline(Deadline&&) = delete;
    virtual ~Deadline() = default;
    Deadline& operator=(const Deadline&) = delete;
    Deadline& operator=(Deadline&&) = delete;

    /// @brief Starts the deadline and returns the handle that stops it.
    /// @return The handle of the running deadline, or one of these errors:
    /// `ErrorCode::kDeadlineRunning` if a handle of this deadline exists, without a change to the running deadline.
    /// `ErrorCode::kDeadlineFailed` if the deadline missed its range earlier.
    score::Result<DeadlineHandle> Start() noexcept;

  protected:
    Deadline() = default;

  private:
    friend class DeadlineHandle;

    /// @brief Records the start time.
    /// @return An empty result, or `ErrorCode::kDeadlineFailed` if the deadline missed its range earlier.
    virtual score::Result<void> DoStart() noexcept = 0;

    /// @brief Records the stop time and checks it against the range.
    virtual void DoStop() noexcept = 0;

    /// @brief True while a handle of this deadline exists.
    std::atomic<bool> running_{false};
};

/// @brief Running deadline, stopped by `Stop()` or by the destructor.
/// @pre The handle is destroyed before the `Taken<Deadline>` it was started from.
/// @note The handle can move to another thread.
class DeadlineHandle final
{
  public:
    DeadlineHandle(const DeadlineHandle&) = delete;

    /// @brief Takes over the running deadline. `other` then does nothing.
    DeadlineHandle(DeadlineHandle&& other) noexcept;

    /// @brief Stops the deadline if this handle still runs it.
    ~DeadlineHandle();

    DeadlineHandle& operator=(const DeadlineHandle&) = delete;
    DeadlineHandle& operator=(DeadlineHandle&&) = delete;

    /// @brief Stops the deadline. A second call and a call on a moved-from handle do nothing.
    void Stop() noexcept;

  private:
    friend class Deadline;

    explicit DeadlineHandle(Deadline& deadline) noexcept;

    /// @brief Running deadline, or `nullptr` after `Stop()` or a move.
    Deadline* deadline_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_H

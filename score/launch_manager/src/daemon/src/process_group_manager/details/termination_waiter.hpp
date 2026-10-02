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

#ifndef _INCLUDED_TERMINATION_WAITER_
#define _INCLUDED_TERMINATION_WAITER_

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"
#include <memory>
#include <mutex>

namespace score::mw::lifecycle::internal
{

/// @brief Allow a thread to wait for a process to terminate.
/// @note Notification of a terminated process must be provided by calling terminated(),
/// from a seperate thread from the waiter.
/// @note Compatible with stop tokens. A stopped token will unblock the waiting thread.
class TerminationWaiter
{
  public:
    /// @brief Constructor
    TerminationWaiter();
    /// @brief Destructor
    ~TerminationWaiter();

    /// @brief Deleted copy constructor (this class is not copyable by design)
    /// @note @ref TerminationWaiter is not copyable by design
    TerminationWaiter(const TerminationWaiter&) = delete;
    /// @brief Deleted copy assignment operator
    /// @note @ref TerminationWaiter is not copyable by design
    TerminationWaiter& operator=(const TerminationWaiter&) = delete;

    /// @brief Deleted move constructor (this class is not movable by design)
    /// @note @ref TerminationWaiter is not movable by design
    TerminationWaiter(TerminationWaiter&& other) = delete;
    /// @brief Deleted move assignment operator
    /// @note @ref TerminationWaiter is not movable by design
    TerminationWaiter& operator=(TerminationWaiter&& other) = delete;

    /// @brief Should be called when a process terminates, to unblock the wait_with_timeout call.
    /// @note As wait_with_timeout is blocking, a call to terminated() should be made from another thread.
    /// @note May throw when locking
    void terminated() noexcept(false);

    /// @brief Wait for termination of the process, timeout, or stop via stop_token.
    bool wait_with_timeout(const score::cpp::stop_token& stop_token, const std::chrono::milliseconds& rel_timeout_ms);

  private:
    /// @brief A struct containing the state shared between the waiting and notifying threads
    struct SharedState
    {
        std::mutex state_mutex;
        score::concurrency::InterruptiblePromise<void> promise;
        score::concurrency::InterruptibleSharedFuture<void> shared_future;
        bool is_terminated{false};

        SharedState();
    };

    /// @brief A shared state allocated on the heap
    std::shared_ptr<SharedState> state_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // _INCLUDED_TERMINATION_WAITER_

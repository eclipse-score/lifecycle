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

#ifndef SCORE_LCM_PROCESS_TERMINATED_READY_CONDITION_HPP_INCLUDED
#define SCORE_LCM_PROCESS_TERMINATED_READY_CONDITION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/termination_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/result/result.h"
#include <atomic>
#include <chrono>
#include <cstdint>

namespace score::mw::lifecycle::internal
{

/// @brief A ready condition which waits for a process to terminate successfully.
/// @details The process is ready once it has terminated with exit status 0. Termination
///          is reported through handleTermination(), which may happen before or during wait().
///          A termination with any other exit status makes wait() return an error.
class ProcessTerminatedReadyCondition final : public IReadyCondition
{
  public:
    /// @brief Creates a new process terminated ready condition.
    ProcessTerminatedReadyCondition() = default;

    /// @brief Prepare the ready condition for a new activation.
    /// @details Discards a termination left over from a previous activation.
    void prepare() const override;

    /// @brief Wait until the process represented by the given handle has terminated.
    /// @details A termination reported after prepare() and before this call is not lost.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The process to wait on.
    /// @param timeout The maximum duration to wait for the process to terminate.
    /// @return Success if the process terminated with exit status 0 within the timeout,
    ///         kGeneralError if it terminated with another exit status or a stop was requested,
    ///         kTimeout otherwise.
    Result<void> wait(cpp::stop_token stop_token, const Handle& handle, std::chrono::milliseconds timeout)
        const override;

    /// @brief Notify the ready condition that a POSIX process has terminated.
    /// @param handle The process that terminated.
    /// @details Wakes up wait() for every exit status. wait() then decides on the stored status.
    /// @param status Exit code of the process.
    void handleTermination(const Handle& handle, int32_t status) const override;

  private:
    /// @brief Synchronises the terminating thread with the waiting thread.
    /// @note Mutable because the IReadyCondition API is const. The waiter is thread-safe.
    mutable TerminationWaiter waiter_;

    /// @brief Exit status of the terminated process. Stored before the waiter is woken up.
    mutable std::atomic<int32_t> status_{0};
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_TERMINATED_READY_CONDITION_HPP_INCLUDED

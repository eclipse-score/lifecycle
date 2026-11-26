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

#ifndef SCORE_LCM_SEQUENCE_READY_CONDITION_HPP_INCLUDED
#define SCORE_LCM_SEQUENCE_READY_CONDITION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/result/result.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace score::mw::lifecycle::internal
{

/// @brief A ready condition which waits for several ready conditions one after another.
/// @details All stages share one ready timeout: the first stage may use the complete timeout, and
///          every following stage gets the time which is left. For example, a reporting process
///          with a file ready condition first reports kRunning, and then the file is waited for.
class SequenceReadyCondition final : public IReadyCondition
{
  public:
    /// @brief Creates a new sequence of ready conditions.
    /// @pre @p stages is not empty and does not contain nullptr.
    /// @param stages The ready conditions to wait for, in this order.
    explicit SequenceReadyCondition(std::vector<std::unique_ptr<const IReadyCondition>> stages);

    /// @brief Prepare every stage for a new activation.
    void prepare() const override;

    /// @brief Wait for every stage in order, until all of them are fulfilled.
    /// @details Fails as soon as a stage fails, with the error of that stage. A stop request only
    ///          interrupts waiting: every remaining stage is still checked once, so the sequence
    ///          succeeds if all of them are already fulfilled.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @param timeout The maximum duration to wait for all stages together.
    /// @return Whether all stages were fulfilled, or the error of the first stage which failed.
    Result<void> wait(cpp::stop_token stop_token, const Handle& handle, std::chrono::milliseconds timeout)
        const override;

    /// @brief Notify the stages which are not fulfilled yet that the process has terminated.
    /// @details The termination is forwarded to the stage currently waited for and to all following
    ///          stages, since each of them still depends on the process. Stages which are already
    ///          fulfilled are not notified. Each notified stage ends its own wait, so the sequence
    ///          ends without waiting for its timeout: a stage which is not fulfilled by the
    ///          termination fails, and a following stage already knows about the termination when
    ///          it is waited for.
    /// @param handle The process that terminated.
    /// @param status Exit code of the process.
    void handleTermination(const Handle& handle, int32_t status) const override;

  private:
    /// @brief The ready conditions to wait for, in this order.
    const std::vector<std::unique_ptr<const IReadyCondition>> stages_;

    /// @brief Index of the stage currently waited for, or 0 while no wait is in progress.
    /// @details Written by wait() on the worker thread, read by handleTermination() on the
    ///          thread reporting the termination. Reset to 0 when the wait ends, so that a termination
    ///          reported before the next wait reaches every stage again.
    /// @note Mutable because the IReadyCondition API is const. A component never runs two
    ///       activations concurrently, so one index per ready condition is sufficient.
    mutable std::atomic<std::size_t> current_stage_{0U};
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_SEQUENCE_READY_CONDITION_HPP_INCLUDED

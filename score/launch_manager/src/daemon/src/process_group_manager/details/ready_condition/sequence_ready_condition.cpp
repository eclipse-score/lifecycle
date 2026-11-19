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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/sequence_ready_condition.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <score/assert.hpp>
#include <algorithm>
#include <utility>

namespace score::mw::lifecycle::internal
{

namespace
{

/// @brief Resets the current stage when the wait ends, however it ends.
class ResetStageOnExit final
{
  public:
    explicit ResetStageOnExit(std::atomic<std::size_t>& current_stage) : current_stage_(current_stage)
    {
    }

    ~ResetStageOnExit()
    {
        current_stage_.store(0U);
    }

    ResetStageOnExit(const ResetStageOnExit&) = delete;
    ResetStageOnExit& operator=(const ResetStageOnExit&) = delete;
    ResetStageOnExit(ResetStageOnExit&&) = delete;
    ResetStageOnExit& operator=(ResetStageOnExit&&) = delete;

  private:
    std::atomic<std::size_t>& current_stage_;
};

}  // namespace

SequenceReadyCondition::SequenceReadyCondition(std::vector<std::unique_ptr<const IReadyCondition>> stages)
    : stages_(std::move(stages))
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(!stages_.empty(), "SequenceReadyCondition requires at least one stage");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        std::none_of(
            stages_.cbegin(),
            stages_.cend(),
            [](const std::unique_ptr<const IReadyCondition>& stage) {
                return stage == nullptr;
            }),
        "SequenceReadyCondition requires every stage to be set");
}

void SequenceReadyCondition::prepare() const
{
    for (const std::unique_ptr<const IReadyCondition>& stage : stages_)
    {
        stage->prepare();
    }
}

Result<void>
SequenceReadyCondition::wait(cpp::stop_token stop_token, const Handle& handle, std::chrono::milliseconds timeout) const
{
    const auto start = std::chrono::steady_clock::now();

    // A termination reported before the next wait starts must reach every stage again.
    const ResetStageOnExit reset_stage{current_stage_};

    // The first stage may use the complete timeout.
    std::chrono::milliseconds remaining = timeout;

    for (std::size_t stage = 0U; stage < stages_.size(); ++stage)
    {
        // Set before the stage starts, so that a termination reported from now on reaches it.
        current_stage_.store(stage);

        // Not every stage reacts to the stop token, so a stop which was requested during the
        // previous stage is checked here.
        if (stop_token.stop_requested())
        {
            return MakeUnexpected(ExecErrc::kGeneralError);
        }

        const Result<void> result = stages_[stage]->wait(stop_token, handle, remaining);
        if (!result.has_value())
        {
            return result;
        }

        // Rounded up, so that all stages together never wait longer than the timeout.
        const auto elapsed = std::chrono::ceil<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        remaining = (elapsed < timeout) ? (timeout - elapsed) : std::chrono::milliseconds::zero();
    }

    return {};
}

void SequenceReadyCondition::handleTermination(const Handle& handle, int32_t status) const
{
    // Every stage which is not fulfilled yet is notified: each of them has to end its wait.
    for (std::size_t stage = current_stage_.load(); stage < stages_.size(); ++stage)
    {
        stages_[stage]->handleTermination(handle, status);
    }
}

}  // namespace score::mw::lifecycle::internal

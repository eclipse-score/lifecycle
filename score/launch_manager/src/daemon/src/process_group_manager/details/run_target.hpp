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

#ifndef SCORE_LCM_RUN_TARGET_HPP_INCLUDED
#define SCORE_LCM_RUN_TARGET_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

#include <atomic>
#include <chrono>

namespace score::mw::lifecycle::internal
{

/// @brief A virtual node in the dependency graph corresponding to a configured Run Target.
/// A RunTarget does not have any resources and is immediately active once requested, provided
/// all of its dependencies are active, and immediately inactive once deactivated.
class RunTarget final : public IComponent
{
  public:
    /// @brief Creates a Run Target node.
    /// @param index Unique identifier of this Run Target.
    /// @param transition_timeout The transition timeout configured for this Run Target.
    explicit RunTarget(const IdentifierHash& index, const std::chrono::milliseconds transition_timeout)
        : identifier_(index), transition_timeout_(transition_timeout)
    {
    }

    RunTarget(RunTarget&& other) noexcept
        : identifier_(other.identifier_), transition_timeout_(other.transition_timeout_), active_(other.active_.load())
    {
    }
    RunTarget(const RunTarget&) = delete;
    RunTarget& operator=(const RunTarget&) = delete;
    RunTarget& operator=(RunTarget&&) = delete;
    ~RunTarget() override = default;

    RequestResult activate(score::cpp::stop_token) override
    {
        active_ = true;
        return RequestResult::kSuccess;
    }

    RequestResult deactivate(score::cpp::stop_token) override
    {
        active_ = false;
        return RequestResult::kSuccess;
    }

    TerminationResult tryHandleTermination(int32_t) override
    {
        return TerminationResult::kIgnore;
    }

    IdentifierHash getIdentifier() const override
    {
        return identifier_;
    }

    bool active() const override
    {
        return active_;
    }

    /// @brief Returns the transition timeout configured for this Run Target.
    /// @details A transition to this Run Target that takes longer than this is
    ///          considered to have failed.
    /// @return The configured transition timeout.
    std::chrono::milliseconds getTransitionTimeout() const noexcept
    {
        return transition_timeout_;
    }

  private:
    /// @brief Unique identifier of this Run Target.
    IdentifierHash identifier_;
    /// @brief Transition timeout configured for this Run Target.
    std::chrono::milliseconds transition_timeout_;
    /// @brief True if the Run Target has been activated and has not yet been deactivated.
    std::atomic<bool> active_{false};
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_RUN_TARGET_HPP_INCLUDED

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

#ifndef SCORE_LCM_CONTROL_PROVIDER
#define SCORE_LCM_CONTROL_PROVIDER

#include "score/mw/launch_manager/process_group_manager/irun_target_control.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

namespace score::mw::lifecycle::internal
{

/// @brief Provides the mw::com service for state managers to connect to.
/// @details This cannot be moved, because the mw::com callbacks reference
//           the ControlProvider at its original location.
class ControlProvider
{
  public:
    /// @brief Fallible constructor for ControllableGraph.
    static Result<ControlProvider*> Create(IRunTargetControl* graph) noexcept;

    ~ControlProvider() = default;

    // Cannot be moved because callbacks capture the ControlProvider by reference.
    ControlProvider(const ControlProvider&) = delete;
    ControlProvider(ControlProvider&&) = delete;
    ControlProvider& operator=(const ControlProvider&) = delete;
    ControlProvider operator=(ControlProvider&&) = delete;

  private:
    explicit ControlProvider(LmControlSkeleton skeleton, IRunTargetControl* graph) noexcept;

    /// @brief Register the handler for activate_run_target.
    Result<void> setupActivateRunTarget() noexcept;

    /// @brief Handle an activate_run_target request.
    void handleActivateRunTarget(ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) noexcept;

    /// @brief Register the handler for get_active_run_target.
    Result<void> setupGetActiveRunTarget() noexcept;

    /// @brief Handle a get_active_run_target request.
    void handleGetActiveRunTarget(GetActiveRunTargetResponse& response) noexcept;

    /// @brief Register the handler for activation_result.
    Result<void> setupActivationResult() noexcept;

    /// @brief Handle an activation_result event.
    void handleActivationResult(IdentifierHash state, RunTargetActivationSource source) noexcept;

    /// @brief Make the service available to clients.
    Result<void> offerService() noexcept;

    /// @brief The external `mw::com` interface.
    LmControlSkeleton skeleton_;

    /// @brief The underlying graph implementation.
    IRunTargetControl* graph_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER

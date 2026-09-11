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

#include "score/mw/launch_manager/control/icontrollable_graph.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

namespace score::mw::lifecycle::internal
{

class ControlProvider
{
  public:
    ControlProvider(IControllableGraph* graph);

  private:
    /// @brief Register the handler for activate_run_target.
    void setupActivateRunTarget();

    /// @brief Handle an activate_run_target request.
    void handleActivateRunTarget(ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request);

    /// @brief Register the handler for get_active_run_target.
    void setupGetActiveRunTarget();

    /// @brief Handle a get_active_run_target request.
    void handleGetActiveRunTarget(GetActiveRunTargetResponse& response);

    /// @brief Register the handler for activation_result.
    void setupActivationResult();

    /// @brief Handle an activation_result event.
    void handleActivationResult(IdentifierHash state, RunTargetActivationSource source);

    /// @brief Make the service available to clients.
    void offerService();

    /// @brief The external `mw::com` interface.
    LmControlSkeleton skeleton_;

    /// @brief The underlying graph implementation.
    IControllableGraph* graph_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER

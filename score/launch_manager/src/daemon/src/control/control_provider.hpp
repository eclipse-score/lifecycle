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
    void setup_activate_run_target();

    /// @brief Handle an activate_run_target request.
    void handle_activate_run_target(ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request);

    /// @brief Register the handler for get_active_run_target.
    void setup_get_active_run_target();

    /// @brief Handle a get_active_run_target request.
    void handle_get_active_run_target(GetActiveRunTargetResponse& response);

    /// @brief Register the handler for activation_result.
    void setup_activation_result();

    /// @brief Handle an activation_result event.
    void handle_activation_result(IdentifierHash state, RunTargetActivationSource source);

    /// @brief Make the service available to clients.
    void offer_service();

    /// @brief The external `mw::com` interface.
    LmControlSkeleton skeleton_;

    /// @brief The underlying graph implementation.
    IControllableGraph* graph_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER

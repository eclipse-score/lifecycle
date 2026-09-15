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

#ifndef SCORE_LCM_ICONTROLLABLE_GRAPH
#define SCORE_LCM_ICONTROLLABLE_GRAPH

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/lifecycle/run_target_activation_source.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

class IControllableGraph
{
  public:
    /// @brief Get the active run target, or an error if we are currently
    ///        in transition.
    [[nodiscard]] virtual score::Result<IdentifierHash> getActiveRunTarget() const = 0;

    /// @brief Set the requested run target.
    [[nodiscard]] virtual score::Result<void> setRequestedRunTarget(IdentifierHash run_target) = 0;

    /// @brief Register a callback to be fired when the active run target changes.
    virtual void registerActiveRunTargetCallback(
        std::function<void(IdentifierHash, RunTargetActivationSource)> callback) = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICONTROLLABLE_GRAPH

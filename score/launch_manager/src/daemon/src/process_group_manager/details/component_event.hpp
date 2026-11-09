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

#ifndef SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED

#include <cstdint>
#include <variant>

#include <chrono>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/completion_slot.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

/// @brief A node finished activating successfully.
/// @details Handled by the Graph to update the state of an ongoing transition
struct [[nodiscard]] ActivationSuccessful
{
    /// @brief The identifier of the node that activated.
    IdentifierHash node_identifier;
};

/// @brief A node failed to activate.
/// @details Handled by the Graph to update the state of an ongoing transition
struct [[nodiscard]] ActivationFailed
{
    /// @brief The identifier of the node that failed.
    IdentifierHash node_identifier;
    /// @brief Why the activation failed, either kFailure or kTimeout.
    IComponent::RequestResult reason;
};

/// @brief A node finished deactivating.
/// @details Handled by the Graph to update the state of an ongoing transition
struct [[nodiscard]] DeactivationComplete
{
    /// @brief The identifier of the node that deactivated.
    IdentifierHash node_identifier;
};

/// @brief A node terminated without having been requested to.
/// @details Handled by the Graph to initiate recovery action
struct [[nodiscard]] UnexpectedTermination
{
    /// @brief The identifier of the node that terminated.
    IdentifierHash node_identifier;
};

/// @brief A job was queued but cancelled by the time it was processed
/// @details Handled by the Graph to keep track of ongoing jobs on the worker threads
struct [[nodiscard]] JobSkipped
{
    /// @brief The identifier of the node that was skipped.
    IdentifierHash node_identifier;
};

/// @brief Alive supervision has failed for the given process identifier.
/// @details Handled by the Graph to initiate recovery action
struct [[nodiscard]] SupervisionFailure
{
    /// @brief The identifier of the node that failed.
    IdentifierHash process_identifier;
    /// @brief Timestamp that pinpoints the supervision failure (on the monotonic clock used by Alive supervision).
    std::chrono::nanoseconds failure_timestamp;
};

/// @brief The state manager wants to know which Run Target is currently active.
/// @details Handled by the ProcessGroupManager to process incoming state manager requests
struct [[nodiscard]] GetActiveRunTarget
{
    CompletionSlot<Result<IdentifierHash>>* completion{nullptr};
};

/// @brief The state manager wants to change to a different Run Target.
/// @details Handled by the ProcessGroupManager to process incoming state manager requests
struct [[nodiscard]] SetRequestedRunTarget
{
    IdentifierHash run_target;
    CompletionSlot<Result<void>>* completion{nullptr};
    /// @brief If true, the request was forced by the user.
    bool forced;
};

/// @brief A graph-relevant state change. There is only ever a single graph, so no process-group
/// identifier is needed to route most events.
using ComponentEvent = std::variant<
    ActivationSuccessful,
    ActivationFailed,
    DeactivationComplete,
    UnexpectedTermination,
    SupervisionFailure,
    JobSkipped,
    GetActiveRunTarget,
    SetRequestedRunTarget>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED

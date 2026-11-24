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

#ifndef SCORE_LCM_ISTART_ACTION_HPP_INCLUDED
#define SCORE_LCM_ISTART_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/icomponent.hpp"
#include "score/result/result.h"
#include <score/stop_token.hpp>
#include <cstdint>
#include <optional>

namespace score::mw::lifecycle::internal
{

/// @brief A resource which was started successfully.
struct StartedResource
{
    /// @brief The handle of the started resource.
    Handle handle{};

    /// @brief Exit status of the resource, if it already terminated before it could be tracked.
    ///        Such a termination is not reported to the component later, so the caller must handle it.
    std::optional<std::int32_t> exit_status{std::nullopt};
};

/// @brief An action which describes how to start a resource.
class IStartAction
{
  public:
    virtual ~IStartAction() = default;

    /// @brief Start the resource and track it, so that its termination is reported to @p component.
    /// @note The termination is never reported from within this call, so it may be called while the
    ///       component's state is locked. If the resource has already terminated, the exit status is
    ///       returned instead.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @param component The component which owns the resource.
    /// @return The started resource, or an error if the action failed. On error, no resource is left
    ///         running.
    virtual Result<StartedResource> start(cpp::stop_token stop_token, IComponent& component) const = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ISTART_ACTION_HPP_INCLUDED

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

#ifndef SCORE_LCM_ISTOP_ACTION_HPP_INCLUDED
#define SCORE_LCM_ISTOP_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/result/result.h"
#include <score/stop_token.hpp>

namespace score::mw::lifecycle::internal
{

/// @brief An action which describes how to gracefully stop a resource.
class IStopAction
{
  public:
    virtual ~IStopAction() = default;

    /// @brief Gracefully stop the resource represented by the given handle.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @param handle The resource to act upon.
    /// @return Whether the action was successful, or failed with an error.
    virtual Result<IComponent::RequestState> stop(cpp::stop_token stop_token, const Handle handle) const = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ISTOP_ACTION_HPP_INCLUDED

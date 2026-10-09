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
#include "score/result/result.h"
#include <score/stop_token.hpp>

namespace score::mw::lifecycle::internal
{

/// @brief An action which describes how to start a resource.
class IStartAction
{
  public:
    virtual ~IStartAction() = default;

    /// @brief Start the resource.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @return The handle of the started resource, or an error if the action failed.
    virtual Result<Handle> start(cpp::stop_token stop_token) const = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ISTART_ACTION_HPP_INCLUDED

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

#ifndef SCORE_LCM_EMPTY_STOP_ACTION_HPP_INCLUDED
#define SCORE_LCM_EMPTY_STOP_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief A stop action which does nothing.
class EmptyStopAction final : public IStopAction
{
  public:
    /// @brief Does nothing and returns success.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @param handle The resource to act upon.
    /// @return Always successful.
    Result<IComponent::RequestState> stop(cpp::stop_token stop_token, const Handle) const override;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_EMPTY_STOP_ACTION_HPP_INCLUDED

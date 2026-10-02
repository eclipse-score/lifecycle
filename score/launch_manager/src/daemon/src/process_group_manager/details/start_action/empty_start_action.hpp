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

#ifndef SCORE_LCM_EMPTY_START_ACTION_HPP_INCLUDED
#define SCORE_LCM_EMPTY_START_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief A start action which does nothing.
class EmptyStartAction final : public IStartAction
{
  public:
    /// @brief Does nothing and returns an empty handle.
    /// @param stop_token Token which can be used to interrupt the action.
    /// @return An empty handle.
    Result<Handle> start(cpp::stop_token stop_token) const override;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_EMPTY_START_ACTION_HPP_INCLUDED

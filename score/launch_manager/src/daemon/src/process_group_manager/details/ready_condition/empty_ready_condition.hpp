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

#ifndef SCORE_LCM_EMPTY_READY_CONDITION_HPP_INCLUDED
#define SCORE_LCM_EMPTY_READY_CONDITION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief A ready condition which is fulfilled immediately.
/// @details Used for resources which are considered ready as soon as they
///          have been started, e.g. native (non-reporting) processes whose
///          ready condition is `Running`, or synchronisation points which do
///          not manage any resource at all.
class EmptyReadyCondition final : public IReadyCondition
{
  public:
    /// @brief Does not wait and returns success.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @return Always successful.
    Result<void> wait(cpp::stop_token stop_token, const Handle& handle) const override;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_EMPTY_READY_CONDITION_HPP_INCLUDED

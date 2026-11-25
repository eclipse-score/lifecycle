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

#ifndef SCORE_LCM_IREADY_CONDITION_HPP_INCLUDED
#define SCORE_LCM_IREADY_CONDITION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/result/result.h"
#include <score/stop_token.hpp>
#include <chrono>

namespace score::mw::lifecycle::internal
{

/// @brief A condition which decides when a resource has finished its startup.
class IReadyCondition
{
  public:
    virtual ~IReadyCondition() = default;

    /// @brief Prepare the ready condition for a new activation.
    /// @details Called once per activation, before the resource is started and while no
    ///          termination of the new resource can be reported yet. Implementations can use
    ///          this to discard state left over from a previous activation.
    virtual void prepare() const
    {
    }

    /// @brief Wait until the resource represented by the given handle is ready.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @param timeout The maximum duration to wait for the condition. A timeout of zero still
    ///                checks the condition once.
    /// @return Nothing if the condition is met, otherwise one of these errors:
    ///         - ExecErrc::kTimeout if the condition was not met within the timeout.
    ///         - ExecErrc::kGeneralError if the wait was interrupted by a stop request or a termination
    ///           which does not fulfil the condition, or if it failed for any other reason.
    virtual Result<void> wait(cpp::stop_token stop_token, const Handle& handle, std::chrono::milliseconds timeout)
        const = 0;

    /// @brief Notify the ready condition that the component has already terminated.
    /// @details Called after prepare(), possibly before, during or after wait() of the same activation.
    ///          A wait() which has not returned yet must then return without waiting for its timeout:
    ///          successfully if the termination fulfils the condition, with an error otherwise.
    ///          Only a ready condition whose wait() does not block may ignore terminations.
    /// @param handle The component that terminated.
    /// @param status Exit code of the component.
    // FIXME: This should be a more generic event which makes sense for any
    //        handle type.
    virtual void handleTermination(const Handle& /*handle*/, int32_t /*status*/) const
    {
    }
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_IREADY_CONDITION_HPP_INCLUDED

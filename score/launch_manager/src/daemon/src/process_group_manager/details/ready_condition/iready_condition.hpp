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

namespace score::mw::lifecycle::internal
{

/// @brief A condition which decides when a resource has finished its startup.
class IReadyCondition
{
  public:
    virtual ~IReadyCondition() = default;

    /// @brief Wait until the resource represented by the given handle is ready.
    /// @param stop_token Token which can be used to interrupt the wait.
    /// @param handle The resource to wait on.
    /// @return Whether the wait was successful, or failed with an error.
    virtual Result<void> wait(cpp::stop_token stop_token, const Handle handle) const = 0;

    /// @brief Notify the ready condition that a POSIX process has terminated.
    /// @param status Exit code of the process.
    /// @return Whether this ready condition accepts the event.
    bool tryHandleTermination(int32_t status) const
    {
        return false;
    }
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_IREADY_CONDITION_HPP_INCLUDED

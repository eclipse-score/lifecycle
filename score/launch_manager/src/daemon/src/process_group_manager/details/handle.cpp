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

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"

namespace score::mw::lifecycle::internal
{

void invalidate(Handle& handle)
{
    if (ProcessHandle* const process_handle = std::get_if<ProcessHandle>(&handle))
    {
        process_handle->pid = -1;
    }
}

bool isAlive(const Handle& handle)
{
    const ProcessHandle* const process_handle = std::get_if<ProcessHandle>(&handle);
    return (process_handle != nullptr) && (process_handle->pid != -1);
}

bool isSameResource(const Handle& lhs, const Handle& rhs)
{
    const ProcessHandle* const lhs_process = std::get_if<ProcessHandle>(&lhs);
    const ProcessHandle* const rhs_process = std::get_if<ProcessHandle>(&rhs);
    if ((lhs_process == nullptr) || (rhs_process == nullptr))
    {
        return false;
    }
    // An invalidated handle no longer refers to a resource, so it cannot refer to the same one as another handle.
    return (lhs_process->pid != -1) && (lhs_process->pid == rhs_process->pid);
}

}  // namespace score::mw::lifecycle::internal

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

#include "score/mw/launch_manager/osal/start_process.hpp"
#include "score/mw/launch_manager/osal/details/qnx/start_process_internal.hpp"

namespace score::mw::lifecycle::internal::osal
{

OsalReturnType
startProcess(ProcessID& pid, const configuration::ComponentConfig& config, int comms_fd, const IpcCommsP& comms)
{
    return detail::startProcessImpl(detail::SpawnOs{}, pid, config, comms_fd, comms);
}

}  // namespace score::mw::lifecycle::internal::osal

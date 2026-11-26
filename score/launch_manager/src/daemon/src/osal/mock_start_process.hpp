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

#ifndef OSAL_MOCK_START_PROCESS_HPP_INCLUDED
#define OSAL_MOCK_START_PROCESS_HPP_INCLUDED

#include <gmock/gmock.h>

#include <memory>

#include "score/mw/launch_manager/osal/start_process.hpp"

// NOLINTBEGIN

/// @brief Mock for osal::startProcess().
/// @details This header defines osal::startProcess() and replaces the real implementation at link time, so it must
/// be included in exactly one translation unit of a statically linked test.
class StartProcessMock
{
  public:
    MOCK_METHOD(
        score::mw::lifecycle::internal::osal::OsalReturnType,
        startProcess,
        (score::mw::lifecycle::internal::osal::ProcessID & pid,
         const score::mw::lifecycle::internal::configuration::ComponentConfig& config,
         int comms_fd,
         const score::mw::lifecycle::internal::osal::IpcCommsP& comms),
        ());
};

std::unique_ptr<StartProcessMock> g_start_process_mock = nullptr;

namespace score::mw::lifecycle::internal::osal
{
OsalReturnType
startProcess(ProcessID& pid, const configuration::ComponentConfig& config, int comms_fd, const IpcCommsP& comms)
{
    return g_start_process_mock->startProcess(pid, config, comms_fd, comms);
}
}  // namespace score::mw::lifecycle::internal::osal

// NOLINTEND

#endif  // OSAL_MOCK_START_PROCESS_HPP_INCLUDED

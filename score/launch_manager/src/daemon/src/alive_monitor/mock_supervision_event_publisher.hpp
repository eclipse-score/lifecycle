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
#ifndef MOCK_ALIVE_SUPERVISION_HANDLE_HPP_INCLUDED
#define MOCK_ALIVE_SUPERVISION_HANDLE_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/ialive_supervision_handle.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle
{

class MockAliveSupervisionHandle : public IAliveSupervisionHandle
{
  public:
    MOCK_METHOD(bool, activateSupervision, (timespec time), (override, noexcept));
    MOCK_METHOD(bool, deactivateSupervision, (timespec time), (override, noexcept));
    MOCK_METHOD(std::string_view, getConnectionId, (), (const, override, noexcept));
};

}  // namespace score::mw::lifecycle

#endif  // MOCK_ALIVE_SUPERVISION_HANDLE_HPP_INCLUDED

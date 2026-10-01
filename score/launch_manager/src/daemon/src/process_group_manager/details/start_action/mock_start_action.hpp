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

#ifndef MOCK_START_ACTION_HPP_INCLUDED
#define MOCK_START_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockStartAction : public IStartAction
{
  public:
    MOCK_METHOD(Result<Handle>, start, (cpp::stop_token), (override, const));
};

}  // namespace score::mw::lifecycle::internal

#endif  // MOCK_START_ACTION_HPP_INCLUDED

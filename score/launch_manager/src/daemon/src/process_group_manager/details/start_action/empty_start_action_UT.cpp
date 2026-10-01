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

#include "score/mw/launch_manager/process_group_manager/details/start_action/empty_start_action.hpp"
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

TEST(EmptyStartActionTest, StartSucceeds)
{
    EmptyStartAction empty_start_action;

    const auto result = empty_start_action.start(cpp::stop_token{});

    EXPECT_TRUE(result.has_value());
}

TEST(EmptyStartActionTest, StartReturnsEmptyHandle)
{
    EmptyStartAction empty_start_action;

    const auto result = empty_start_action.start(cpp::stop_token{});

    EXPECT_TRUE(std::holds_alternative<EmptyHandle>(result.value()));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

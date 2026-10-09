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

#include "score/mw/launch_manager/process_group_manager/details/stop_action/empty_stop_action.hpp"
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

TEST(EmptyStopActionTest, StopSucceeds)
{
    EmptyStopAction empty_stop_action;

    const Result<IComponent::RequestState> result = empty_stop_action.stop(cpp::stop_token{}, EmptyHandle{});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IComponent::RequestState::kSuccess);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

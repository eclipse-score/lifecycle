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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/empty_ready_condition.hpp"
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

TEST(EmptyReadyConditionTest, WaitWithEmptyHandleSucceeds)
{
    const EmptyReadyCondition empty_ready_condition;

    const Result<void> result = empty_ready_condition.wait(cpp::stop_token{}, EmptyHandle{});

    EXPECT_TRUE(result.has_value());
}

TEST(EmptyReadyConditionTest, WaitWithProcessHandleSucceeds)
{
    const EmptyReadyCondition empty_ready_condition;

    const Result<void> result = empty_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr});

    EXPECT_TRUE(result.has_value());
}

TEST(EmptyReadyConditionTest, DoesNotHandleTermination)
{
    const EmptyReadyCondition empty_ready_condition;

    EXPECT_FALSE(empty_ready_condition.tryHandleTermination(ProcessHandle{42, nullptr}, 0));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

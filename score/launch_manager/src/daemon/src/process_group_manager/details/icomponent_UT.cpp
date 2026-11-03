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

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/mock_component.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

TEST(IComponentTest, DefaultHandleSupervisionFailureDoesNothing)
{
    // MockComponent does not override handleSupervisionFailure, so the default implementation is called.
    // StrictMock fails the test if the default implementation calls any other method of the component.
    StrictMock<MockComponent> component;
    IComponent& icomponent = component;

    EXPECT_NO_THROW(icomponent.handleSupervisionFailure(std::chrono::nanoseconds{42}));
}

TEST(IComponentTest, HandleSupervisionFailureIsForwardedToSubclass)
{
    StrictMock<MockSupervisedComponent> component;
    IComponent& icomponent = component;

    EXPECT_CALL(component, handleSupervisionFailure(std::chrono::nanoseconds{42})).Times(1);

    icomponent.handleSupervisionFailure(std::chrono::nanoseconds{42});
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

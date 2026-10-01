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

#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/mock_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/mock_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/mock_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/mock_stop_action.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <score/stop_token.hpp>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

const ProcessHandle mock_handle = ProcessHandle{42};

const cpp::stop_source mock_stop_source;
const cpp::stop_token mock_stop_token = mock_stop_source.get_token();

void expect_mock_handle(const Handle handle)
{
    EXPECT_EQ(std::get<ProcessHandle>(handle).pid, mock_handle.pid);
}

void expect_mock_stop_token(const cpp::stop_token token)
{
    EXPECT_EQ(token, mock_stop_token);
}

TEST(ComponentTest, StartSucceeds)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_TRUE(result.has_value());
}

TEST(ComponentTest, StartActionCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    EXPECT_CALL(start_action, start(_))
        .WillOnce(DoAll(WithArg<0>(Invoke(expect_mock_stop_token)), Return(Result<Handle>{mock_handle})));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, StopActionCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(stop_action, stop(_, _))
        .WillOnce(DoAll(
            WithArg<0>(Invoke(expect_mock_stop_token)),
            WithArg<1>(Invoke(expect_mock_handle)),
            Return(Result<void>{})));

    static_cast<void>(component.activate(cpp::stop_token{}));
    component.deactivate(mock_stop_token);
}

TEST(ComponentTest, ReadyConditionsCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockReadyCondition ready_condition_1;
    MockReadyCondition ready_condition_2;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {&ready_condition_1, &ready_condition_2});

    InSequence sequence;
    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition_1, wait(_, _))
        .WillOnce(DoAll(
            WithArg<0>(Invoke(expect_mock_stop_token)),
            WithArg<1>(Invoke(expect_mock_handle)),
            Return(Result<void>{})));
    EXPECT_CALL(ready_condition_2, wait(_, _))
        .WillOnce(DoAll(
            WithArg<0>(Invoke(expect_mock_stop_token)),
            WithArg<1>(Invoke(expect_mock_handle)),
            Return(Result<void>{})));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, StartSetsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, StopClearsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    static_cast<void>(component.activate(cpp::stop_token{}));
    component.deactivate(cpp::stop_token{});

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, ForceStopClearsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    Component component(&start_action, &stop_action, &force_stop_action, {});

    static_cast<void>(component.activate(cpp::stop_token{}));
    component.deactivate(cpp::stop_token{});

    EXPECT_FALSE(component.active());
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

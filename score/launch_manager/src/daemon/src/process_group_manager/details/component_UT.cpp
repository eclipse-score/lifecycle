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
#include <thread>

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

IComponent::TerminationResult terminateFromOtherThread(Component& component, const int32_t status)
{
    // Termination notifications come from another thread in reality, so
    // replacate it in the test to catch concurrency problems.

    IComponent::TerminationResult result;
    std::thread termination_thread([&component, &result, status]() {
        result = component.tryHandleTermination(status);
    });
    termination_thread.join();
    return result;
}

TEST(ComponentTest, StartSucceeds)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, StartActionCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    EXPECT_CALL(start_action, start(_)).WillOnce(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, StopActionCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(stop_action, stop(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(IComponent::RequestResult::kSuccess)));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(mock_stop_token));
}

TEST(ComponentTest, DeactivateReturnsSuccessWhenStopActionReturnsSuccess)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, DeactivateReturnsFailureWhenStopActionFails)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kFailure));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, ReadyConditionCalled)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(Result<void>{})));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, TerminationDuringStartupInterruptsActivation)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&component](cpp::stop_token token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 1));

        // As a result of the termination, the ready condition is stopped.
        EXPECT_TRUE(token.stop_requested());
        return Result<void>{};
    }));

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, StartSetsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, StopClearsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CannotDeactivateInTerminatedState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, CannotActivateInReadyState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotActivateInTerminatingState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateInTerminatingState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, CannotActivateInFaultState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateInFaultState)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsIgnored)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(0);

    EXPECT_EQ(result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(0));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsFailure)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(1);

    EXPECT_EQ(result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsNotActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsIgnored)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    IComponent::TerminationResult result;
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        result = terminateFromOtherThread(component, 0);
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_EQ(result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 0));
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInStartingStateIsFailure)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    IComponent::TerminationResult result;
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        result = terminateFromOtherThread(component, 1);
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_EQ(result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInStartingStateIsNotActive)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition, true);

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 1));
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SettingBothStopTokens)
{
    MockStartAction start_action;
    MockStopAction stop_action;
    MockForceStopAction force_stop_action;
    MockReadyCondition ready_condition;
    Component component(start_action, stop_action, force_stop_action, ready_condition);
    cpp::stop_source stop_source{};

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 1));
        EXPECT_TRUE(token.stop_requested());

        static_cast<void>(stop_source.request_stop());
        EXPECT_TRUE(token.stop_requested());

        return Result<void>{};
    }));

    const auto result = component.activate(stop_source.get_token());

    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

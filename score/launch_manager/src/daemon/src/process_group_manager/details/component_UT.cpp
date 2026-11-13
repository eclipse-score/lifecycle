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

#include "score/mw/launch_manager/alive_monitor/mock_alive_supervision_handle.hpp"
#include "score/mw/launch_manager/alive_monitor/mock_supervision_factory.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/mock_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/mock_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/process_terminated_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/mock_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/mock_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <score/stop_token.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <future>
#include <thread>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

const ProcessHandle mock_handle = ProcessHandle{42, nullptr};

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
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, StartActionCalled)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).WillOnce(Return(Result<Handle>{mock_handle}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, StopActionCalled)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(IComponent::RequestResult::kSuccess)));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(mock_stop_token));
}

TEST(ComponentTest, DeactivateReturnsSuccessWhenStopActionReturnsSuccess)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, DeactivateReturnsFailureWhenStopActionFails)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kFailure));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, ReadyConditionCalled)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(Result<void>{})));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, ReadyConditionPreparedBeforeStart)
{
    RecordProperty(
        "Description",
        "Given a component. "
        "When it is activated. "
        "Then the ready condition is prepared before the resource is started, and waited on afterwards.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    auto* const start_action_ptr = start_action.get();

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    InSequence sequence;
    EXPECT_CALL(*ready_condition_ptr, prepare());
    EXPECT_CALL(*start_action_ptr, start(_)).WillOnce(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Return(Result<void>{}));

    EXPECT_EQ(component.activate(mock_stop_token), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, ReadyConditionTimeoutReturnsTimeout)
{
    RecordProperty(
        "Description",
        "Given a component whose ready condition times out. "
        "When it is activated. "
        "Then activation returns kTimeout.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    EXPECT_CALL(*ready_condition, wait(_, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_EQ(component.activate(mock_stop_token), IComponent::RequestResult::kTimeout);
}

TEST(ComponentTest, ReadyConditionErrorReturnsFailure)
{
    RecordProperty(
        "Description",
        "Given a component whose ready condition fails with an error other than a timeout. "
        "When it is activated. "
        "Then activation returns kFailure.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    EXPECT_CALL(*ready_condition, wait(_, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_EQ(component.activate(mock_stop_token), IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, TerminationDuringStartupInterruptsActivation)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&component](cpp::stop_token token, const Handle) {
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
    NiceMock<MockSupervisionFactory> factory;

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, StopClearsActive)
{
    NiceMock<MockSupervisionFactory> factory;

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CannotDeactivateInTerminatedState)
{
    NiceMock<MockSupervisionFactory> factory;

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, CannotActivateInReadyState)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotActivateInTerminatingState)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, DeactivateWithEmptyHandleIsInactiveImmediately)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given an active component whose start action returned an empty handle. "
        "When the component is deactivated. "
        "Then deactivation succeeds and the component is not active without waiting for a termination.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{EmptyHandle{}}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CanActivateAgainAfterDeactivateWithEmptyHandle)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a component whose start action returns an empty handle, which has been activated and deactivated. "
        "When the component is activated again. "
        "Then activation succeeds and the component is active.");

    NiceMock<MockSupervisionFactory> factory;

    // Without a resource there is no termination to wait for, so the component
    // goes straight back to the terminated state and can be activated again.
    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).Times(2).WillRepeatedly(Return(Result<Handle>{EmptyHandle{}}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));
    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, DeactivateWithEmptyHandleFailsWhenStopActionFails)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given an active component whose start action returned an empty handle. "
        "When the component is deactivated and the stop action fails. "
        "Then deactivation fails and the component is not active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{EmptyHandle{}}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kFailure));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CannotDeactivateInTerminatingState)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    ON_CALL(*stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, CannotActivateInFaultState)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateInFaultState)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsIgnored)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(0);

    EXPECT_EQ(result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsActive)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(0));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsFailure)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(1);

    EXPECT_EQ(result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsNotActive)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsIgnored)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    IComponent::TerminationResult result;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        result = terminateFromOtherThread(component, 0);
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_EQ(result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsActive)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 0));
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInStartingStateIsFailure)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition_mock = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition_mock.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition_mock),
        factory,
        std::move(config));

    IComponent::TerminationResult result;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        result = terminateFromOtherThread(component, 1);
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_EQ(result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInStartingStateIsNotActive)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 1));
        return Result<void>{};
    }));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SettingBothStopTokens)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    cpp::stop_source stop_source{};

    EXPECT_CALL(*ready_condition_ptr, wait(_, _)).WillOnce(Invoke([&](cpp::stop_token token, const Handle) {
        static_cast<void>(terminateFromOtherThread(component, 1));
        EXPECT_TRUE(token.stop_requested());

        static_cast<void>(stop_source.request_stop());
        EXPECT_TRUE(token.stop_requested());

        return Result<void>{};
    }));

    const auto result = component.activate(stop_source.get_token());

    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, SupervisionConstructedFromConfig)
{
    NiceMock<MockSupervisionFactory> factory;

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->name = "comp";
    config->deployment_config.sandbox.uid = 1000U;
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};
    config->component_properties.application_profile.alive_supervision->reporting_cycle_ms = 50U;

    EXPECT_CALL(
        factory,
        constructSupervision(
            IdentifierHash{"comp"}, 1000U, Field(&configuration::ComponentAliveSupervision::reporting_cycle_ms, 50U)))
        .WillOnce(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
}

TEST(ComponentTest, NoSupervisionConstructedWhenUnsupervised)
{
    NiceMock<MockSupervisionFactory> factory;

    auto config = std::make_shared<configuration::ComponentConfig>();
    const auto* const config_ptr = config.get();
    config->component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;

    EXPECT_CALL(factory, constructSupervision(_, _, _)).Times(0);

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_EQ(config_ptr->deployment_config.environmental_variables.size(), 0U);
}

TEST(ComponentTest, AliveInterfacePathAddedToEnvironment)
{
    NiceMock<MockSupervisionFactory> factory;

    auto config = std::make_shared<configuration::ComponentConfig>();
    const auto* const config_ptr = config.get();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    ON_CALL(*handle, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    ASSERT_EQ(config_ptr->deployment_config.environmental_variables.size(), 1U);
    const auto& variable = *config_ptr->deployment_config.environmental_variables.begin();
    EXPECT_EQ(variable.key(), "LCM_ALIVE_INTERFACE_PATH");
    EXPECT_EQ(variable.value(), "/test_path");
}

TEST(ComponentTest, FailedSupervisionConstructionFailsActivation)
{
    NiceMock<MockSupervisionFactory> factory;

    auto config = std::make_shared<configuration::ComponentConfig>();
    const auto* const config_ptr = config.get();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
    EXPECT_EQ(config_ptr->deployment_config.environmental_variables.size(), 0U);
}

TEST(ComponentTest, SupervisionActivatedWhenReady)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionActivationFailureFaultsComponent)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(false));

    const auto result = component.activate(cpp::stop_token{});
    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, SupervisionNotActivatedWhenStartFails)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{MakeUnexpected(ExecErrc::kGeneralError)}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).Times(0);

    static_cast<void>(component.activate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivatedImmediatelyOnTermination)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    InSequence sequence;
    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(true));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    // deactivate() is never called because there is nothing reading the event queue
}

TEST(ComponentTest, SupervisionDeactivatedBeforeStopAction)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    ON_CALL(*supervision, activateSupervision(_)).WillByDefault(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));

    InSequence sequence;
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(true));

    static_cast<void>(component.deactivate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivationFailureFaultsComponent)
{
    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    ON_CALL(*start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).Times(0);

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));
    static_cast<void>(component.activate(cpp::stop_token{}));
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(false));

    const auto result = component.deactivate(cpp::stop_token{});
    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, ProcessTerminatedReadyConditionSucceedsOnExitZero)
{
    RecordProperty(
        "Description",
        "Given a component with a ProcessTerminatedReadyCondition. "
        "When the process exits with status 0 during activation. "
        "Then the termination is ignored and activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    std::promise<void> started;
    auto started_future = started.get_future();

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).WillOnce(Invoke([&started](cpp::stop_token) {
        started.set_value();
        return Result<Handle>{mock_handle};
    }));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<ProcessTerminatedReadyCondition>(std::chrono::milliseconds{5000}),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    auto activation = std::async(std::launch::async, [&component] {
        return component.activate(cpp::stop_token{});
    });

    started_future.wait();
    const IComponent::TerminationResult termination_result = terminateFromOtherThread(component, 0);

    ASSERT_EQ(activation.wait_for(std::chrono::seconds{1}), std::future_status::ready);
    EXPECT_EQ(activation.get(), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, ProcessTerminatedReadyConditionFailsOnNonZeroExit)
{
    RecordProperty(
        "Description",
        "Given a component with a ProcessTerminatedReadyCondition. "
        "When the process exits with a non-zero status during activation. "
        "Then the termination is a failure and activation fails without waiting for the timeout.");

    NiceMock<MockSupervisionFactory> factory;

    std::promise<void> started;
    auto started_future = started.get_future();

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).WillOnce(Invoke([&started](cpp::stop_token) {
        started.set_value();
        return Result<Handle>{mock_handle};
    }));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<ProcessTerminatedReadyCondition>(std::chrono::milliseconds{5000}),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    auto activation = std::async(std::launch::async, [&component] {
        return component.activate(cpp::stop_token{});
    });

    started_future.wait();
    const IComponent::TerminationResult termination_result = terminateFromOtherThread(component, 1);

    ASSERT_EQ(activation.wait_for(std::chrono::seconds{1}), std::future_status::ready);
    EXPECT_EQ(activation.get(), IComponent::RequestResult::kFailure);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, ReportRunningProcessDyingDuringHandshakeFailsActivation)
{
    RecordProperty(
        "Description",
        "Given a component with a ReportRunningReadyCondition waiting for kRunning. "
        "When the process dies and the termination completes the kRunning handshake. "
        "Then activation fails instead of reporting the dead process as running.");

    // tryHandleTermination posts on the real send semaphore.
    alignas(osal::IpcCommsSync) std::array<std::byte, sizeof(osal::IpcCommsSync)> buffer{};
    const osal::IpcCommsP sync{reinterpret_cast<osal::IpcCommsSync*>(buffer.data()), [](osal::IpcCommsSync*) {
                               }};
    ASSERT_EQ(sync->send_sync_.init(0U, false), osal::OsalReturnType::kSuccess);
    const ProcessHandle handle{42, sync};

    NiceMock<MockSupervisionFactory> factory;
    NiceMock<osal::MockIProcess> launcher;

    std::promise<void> waiting;
    auto waiting_future = waiting.get_future();
    std::promise<void> terminated;
    auto terminated_future = terminated.get_future();

    // Simulates the case where the post from tryHandleTermination completes the handshake.
    EXPECT_CALL(launcher, waitForkRunning(_, _))
        .WillOnce(Invoke([&waiting, &terminated_future](osal::IpcCommsP, std::optional<std::chrono::milliseconds>) {
            waiting.set_value();
            terminated_future.wait();
            return osal::OsalReturnType::kSuccess;
        }));

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).WillOnce(Return(Result<Handle>{handle}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<ReportRunningReadyCondition>(launcher, std::chrono::milliseconds{5000}),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    auto activation = std::async(std::launch::async, [&component] {
        return component.activate(cpp::stop_token{});
    });

    // The worker is blocked in waitForkRunning, with the state lock released.
    waiting_future.wait();
    const IComponent::TerminationResult termination_result = terminateFromOtherThread(component, 1);
    terminated.set_value();

    ASSERT_EQ(activation.wait_for(std::chrono::seconds{1}), std::future_status::ready);
    EXPECT_EQ(activation.get(), IComponent::RequestResult::kFailure);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kIgnore);

    static_cast<void>(sync->send_sync_.deinit());
}

/// @brief Expects one stop of an already terminated process, which must not be asked to terminate again.
void expectStopOfDeadProcess(MockStopAction& stop_action)
{
    EXPECT_CALL(stop_action, stop(_, _)).WillOnce(Invoke([](cpp::stop_token, const Handle& handle) {
        EXPECT_FALSE(isAlive(handle));
        return Result<IComponent::RequestResult>{IComponent::RequestResult::kSuccess};
    }));
}

TEST(ComponentTest, LateTerminationAfterReadyTimeoutDoesNotBlockReactivation)
{
    RecordProperty(
        "Description",
        "Given a ProcessTerminatedReadyCondition whose wait timed out. "
        "When the process exits with 0 afterwards and the component is deactivated. "
        "Then the component is terminated immediately and can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    std::promise<void> second_start;
    auto second_start_future = second_start.get_future();

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_))
        .WillOnce(Return(Result<Handle>{mock_handle}))
        .WillOnce(Invoke([&second_start](cpp::stop_token) {
            second_start.set_value();
            return Result<Handle>{mock_handle};
        }));

    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<ProcessTerminatedReadyCondition>(std::chrono::milliseconds{100}),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    // The ready timeout expires, then the process exits with 0.
    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kTimeout);
    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    // The second activation succeeds once its own process terminates.
    auto activation = std::async(std::launch::async, [&component] {
        return component.activate(cpp::stop_token{});
    });
    second_start_future.wait();
    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);

    ASSERT_EQ(activation.wait_for(std::chrono::seconds{1}), std::future_status::ready);
    EXPECT_EQ(activation.get(), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SelfTerminatingExitWhileStartingDoesNotBlockReactivation)
{
    RecordProperty(
        "Description",
        "Given a self-terminating component whose process exits with 0 while starting. "
        "When the component is deactivated. "
        "Then the component is terminated immediately and can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).Times(2).WillRepeatedly(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    EXPECT_CALL(*ready_condition_ptr, wait(_, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&) {
            EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
            return Result<void>{};
        }))
        .WillOnce(Return(Result<void>{}));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SelfTerminatingExitWhileReadyDoesNotBlockReactivation)
{
    RecordProperty(
        "Description",
        "Given a self-terminating component whose process exits with 0 after it became ready. "
        "When the component is deactivated. "
        "Then the component is terminated immediately and can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_)).Times(2).WillRepeatedly(Return(Result<Handle>{mock_handle}));

    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    auto ready_condition = std::make_unique<MockReadyCondition>();
    ON_CALL(*ready_condition, wait(_, _)).WillByDefault(Return(Result<void>{}));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

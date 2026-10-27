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
#include "score/mw/launch_manager/process_group_manager/details/start_action/mock_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/mock_stop_action.hpp"
#include "score/mw/lifecycle/execution_error.h"
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
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});

    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, StartActionCalled)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    EXPECT_CALL(start_action, start(_)).WillOnce(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, StopActionCalled)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(stop_action, stop(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(IComponent::RequestResult::kSuccess)));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(mock_stop_token));
}

TEST(ComponentTest, DeactivateReturnsSuccessWhenStopActionReturnsSuccess)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, DeactivateReturnsFailureWhenStopActionFails)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kFailure));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, ReadyConditionCalled)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(ready_condition, wait(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(Result<void>{})));

    static_cast<void>(component.activate(mock_stop_token));
}

TEST(ComponentTest, TerminationDuringStartupInterruptsActivation)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

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
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});

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
        configuration::ComponentConfig{});

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
        configuration::ComponentConfig{});

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, CannotActivateInReadyState)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotActivateInTerminatingState)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(stop_action, stop(_, _)).WillByDefault(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateInTerminatingState)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

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
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateInFaultState)
{
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsIgnored)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(0);

    EXPECT_EQ(result, IComponent::TerminationResult::kIgnore);
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsActive)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(0));

    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsFailure)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.tryHandleTermination(1);

    EXPECT_EQ(result, IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsNotActive)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsIgnored)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

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
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

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
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

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
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.is_self_terminating = true;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());

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
    NiceMock<MockSupervisionFactory> factory;
    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        configuration::ComponentConfig{});
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockReadyCondition& ready_condition = static_cast<const MockReadyCondition&>(component.readyCondition());
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

TEST(ComponentTest, SupervisionConstructedFromConfig)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.name = "comp";
    config.deployment_config.sandbox.uid = 1000U;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};
    config.component_properties.application_profile.alive_supervision->reporting_cycle_ms = 50U;

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
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;

    EXPECT_CALL(factory, constructSupervision(_, _, _)).Times(0);

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));

    EXPECT_EQ(component.config().deployment_config.environmental_variables.size(), 0U);
}

TEST(ComponentTest, AliveInterfacePathAddedToEnvironment)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

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

    ASSERT_EQ(component.config().deployment_config.environmental_variables.size(), 1U);
    const auto& variable = *component.config().deployment_config.environmental_variables.begin();
    EXPECT_EQ(variable.key(), "LCM_ALIVE_INTERFACE_PATH");
    EXPECT_EQ(variable.value(), "/test_path");
}

TEST(ComponentTest, FailedSupervisionConstructionFailsActivation)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

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
    EXPECT_EQ(component.config().deployment_config.environmental_variables.size(), 0U);
}

TEST(ComponentTest, SupervisionActivatedWhenReady)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionActivationFailureFaultsComponent)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
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
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{MakeUnexpected(ExecErrc::kGeneralError)}));
    EXPECT_CALL(*supervision, activateSupervision(_)).Times(0);

    static_cast<void>(component.activate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivatedImmediatelyOnTermination)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    InSequence sequence;
    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(true));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(1));

    // deactivate() is never called because there is nothing reading the event queue
}

TEST(ComponentTest, SupervisionDeactivatedBeforeStopAction)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());

    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    ON_CALL(*supervision, activateSupervision(_)).WillByDefault(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));

    InSequence sequence;
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(true));
    EXPECT_CALL(stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    static_cast<void>(component.deactivate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivationFailureFaultsComponent)
{
    NiceMock<MockSupervisionFactory> factory;
    configuration::ComponentConfig config;
    config.component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config.component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    ON_CALL(*supervision, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::move(config));
    const MockStartAction& start_action = static_cast<const MockStartAction&>(component.startAction());
    const MockStopAction& stop_action = static_cast<const MockStopAction&>(component.stopAction());
    ON_CALL(start_action, start(_)).WillByDefault(Return(Result<Handle>{mock_handle}));
    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));
    static_cast<void>(component.activate(cpp::stop_token{}));
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(false));
    EXPECT_CALL(stop_action, stop(_, _)).Times(0);

    const auto result = component.deactivate(cpp::stop_token{});
    EXPECT_EQ(result, IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
    EXPECT_DEATH(
        { static_cast<void>(component.deactivate(cpp::stop_token{})); },
        "Cannot deactivate component in a non-running state");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

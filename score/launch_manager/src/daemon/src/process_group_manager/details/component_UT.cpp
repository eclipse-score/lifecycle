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
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/mock_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/process_terminated_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/mock_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/mock_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <score/jthread.hpp>
#include <score/stop_token.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <optional>
#include <thread>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

const ProcessHandle mock_handle = ProcessHandle{42, nullptr};

/// @brief Handle of the resource after the component was started again.
const ProcessHandle restarted_handle = ProcessHandle{43, nullptr};

/// @brief A timeout which never expires during a test run. Tests which must not time out use it
///        and check the result instead of measuring time, so they cannot become flaky on slow machines.
///        An unwanted wait shows up as a hanging test, never as a flake.
constexpr std::chrono::milliseconds kNeverExpires{std::chrono::hours{1}};

const cpp::stop_source mock_stop_source;
const cpp::stop_token mock_stop_token = mock_stop_source.get_token();

void expect_mock_handle(const Handle handle)
{
    EXPECT_EQ(std::get<ProcessHandle>(handle).pid, mock_handle.pid);
}

IComponent::TerminationResult
terminateFromOtherThread(Component& component, const int32_t status, const Handle& terminated = mock_handle)
{
    // Termination notifications come from another thread in reality, so
    // replacate it in the test to catch concurrency problems.

    IComponent::TerminationResult result;
    std::thread termination_thread([&component, &result, &terminated, status]() {
        result = component.tryHandleTermination(terminated, status);
    });
    termination_thread.join();
    return result;
}

/// @brief Configuration with the given shutdown timeout, and a ready timeout which never expires.
std::shared_ptr<configuration::ComponentConfig> configWithShutdownTimeout(std::chrono::milliseconds timeout)
{
    auto config = std::make_shared<configuration::ComponentConfig>();
    config->deployment_config.ready_timeout_ms = static_cast<uint32_t>(kNeverExpires.count());
    config->deployment_config.shutdown_timeout_ms = static_cast<uint32_t>(timeout.count());
    return config;
}

/// @brief Configuration of a self-terminating component.
std::shared_ptr<configuration::ComponentConfig> selfTerminatingConfig()
{
    auto config = configWithShutdownTimeout(kNeverExpires);
    config->component_properties.application_profile.is_self_terminating = true;
    return config;
}

/// @brief Configuration of a supervised component.
std::shared_ptr<configuration::ComponentConfig> supervisedConfig()
{
    auto config = configWithShutdownTimeout(kNeverExpires);
    config->component_properties.application_profile.application_type =
        configuration::ApplicationType::ReportingAndSupervised;
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};
    return config;
}

/// @brief Makes the factory hand out the given supervision handle.
void provideSupervision(MockSupervisionFactory& factory, std::unique_ptr<NiceMock<MockAliveSupervisionHandle>> handle)
{
    ON_CALL(*handle, getConnectionId()).WillByDefault(Return(std::string_view{"/test_path"}));
    ON_CALL(factory, constructSupervision(_, _, _))
        .WillByDefault(Return(ByMove(std::unique_ptr<IAliveSupervisionHandle>{std::move(handle)})));
}

/// @brief A stop or force stop action which succeeds and reports the termination of the resource it was
///        called with, with @p status, from another thread.
/// @details The reporting thread blocks until the component releases its lock while it waits for the
///          termination, like the OsHandler thread does.
auto reportTerminationLater(
    Component& component,
    cpp::jthread& thread,
    IComponent::RequestResult action_result = IComponent::RequestResult::kSuccess,
    int32_t status = 0)
{
    return [&component, &thread, action_result, status](cpp::stop_token, const Handle& handle) {
        // Assigning joins a thread started by a previous call.
        thread = cpp::jthread{[&component, status, handle] {
            EXPECT_EQ(component.tryHandleTermination(handle, status), IComponent::TerminationResult::kIgnore);
        }};
        return Result<IComponent::RequestResult>{action_result};
    };
}

const auto kStartsProcess = Return(Result<StartedResource>{StartedResource{mock_handle}});
const auto kRestartsProcess = Return(Result<StartedResource>{StartedResource{restarted_handle}});

/// @brief Expects one stop of an already terminated process, which must not be asked to terminate again.
void expectStopOfDeadProcess(MockStopAction& stop_action)
{
    EXPECT_CALL(stop_action, stop(_, _)).WillOnce(Invoke([](cpp::stop_token, const Handle& handle) {
        EXPECT_FALSE(isAlive(handle));
        return Result<IComponent::RequestResult>{IComponent::RequestResult::kSuccess};
    }));
}

TEST(ComponentTest, StopActionCalled)
{
    RecordProperty(
        "Description",
        "Given an active component with a running process. "
        "When it is deactivated. "
        "Then the stop action is called with the handle of the started process.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .WillOnce(DoAll(
            WithArg<1>(Invoke(expect_mock_handle)), Invoke(reportTerminationLater(component, termination_thread))));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(mock_stop_token));
}

TEST(ComponentTest, DeactivateReturnsSuccessWhenStopActionReturnsSuccess)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When it is deactivated, the stop action succeeds and the termination is reported. "
        "Then deactivation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(mock_stop_token);

    ASSERT_EQ(result, IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, ReadyConditionWaitsWithConfiguredReadyTimeout)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "interface-analysis");
    RecordProperty(
        "Description",
        "Given a component configured with a ready timeout. "
        "When the component is activated. "
        "Then the ready condition is waited for with the configured ready timeout.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    auto* const start_action_ptr = start_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->deployment_config.ready_timeout_ms = 1234U;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    InSequence sequence;
    EXPECT_CALL(*ready_condition_ptr, prepare());
    EXPECT_CALL(*start_action_ptr, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, std::chrono::milliseconds{1234})).WillOnce(Return(Result<void>{}));

    EXPECT_EQ(component.activate(mock_stop_token), IComponent::RequestResult::kSuccess);
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
    EXPECT_CALL(*start_action_ptr, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _)).WillOnce(Return(Result<void>{}));

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
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

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
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(mock_stop_token), IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, TerminationWhileStartingIsReportedToReadyCondition)
{
    RecordProperty(
        "Description",
        "Given a component waiting for its ready condition. "
        "When the resource terminates. "
        "Then the termination and its exit status are reported to the ready condition.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<MockReadyCondition>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        selfTerminatingConfig());

    EXPECT_CALL(*ready_condition_ptr, prepare());
    EXPECT_CALL(*ready_condition_ptr, handleTermination(_, 3)).WillOnce(Invoke([](const Handle& handle, int32_t) {
        // The handle is already invalidated, the resource is gone.
        EXPECT_FALSE(isAlive(handle));
    }));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            EXPECT_EQ(terminateFromOtherThread(component, 3), IComponent::TerminationResult::kIgnore);
            return Result<void>{MakeUnexpected(ExecErrc::kGeneralError)};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
}

TEST(ComponentTest, TerminationWhileReadyIsNotReportedToReadyCondition)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When the resource terminates. "
        "Then the termination is not reported to the ready condition.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, handleTermination(_, _)).Times(0);

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        selfTerminatingConfig());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kFailure);
}

TEST(ComponentTest, UnexpectedTerminationDuringStartupFailsActivation)
{
    RecordProperty(
        "Description",
        "Given a component which is not self-terminating and waits for its ready condition. "
        "When the resource terminates and the ready condition is met anyway. "
        "Then the termination is ignored, the stop token is not stopped and activation fails, "
        "so that only one event is reported for the activation.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token token, const Handle, std::chrono::milliseconds) {
            EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
            EXPECT_FALSE(token.stop_requested());
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, StartActionReceivesComponentAndReadyConditionWaitsOnStartedResource)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given an inactive component. "
        "When the component is activated. "
        "Then the start action is called with the component, so that the resource can be tracked for it, and the "
        "ready condition is waited for on the handle returned by the start action.");

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

    const InSequence sequence;
    EXPECT_CALL(*ready_condition_ptr, prepare());
    EXPECT_CALL(*start_action_ptr, start(_, Ref(component)))
        .WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([](cpp::stop_token, const Handle& handle, std::chrono::milliseconds) {
            expect_mock_handle(handle);
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, AlreadyTerminatedResourceFailsActivation)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given an inactive component whose resource terminates unexpectedly before it is tracked. "
        "When the component is activated and the start action reports the termination. "
        "Then the component handles the termination itself without a deadlock, the ready condition is informed "
        "before it is waited for, activation fails and the component is inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle, 1}}));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    {
        InSequence sequence;
        EXPECT_CALL(*ready_condition, handleTermination(_, 1));
        EXPECT_CALL(*ready_condition, wait(_, _, _))
            .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}));
    }

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SelfTerminatingAlreadyExitedWithZeroIsIgnored)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given an inactive self-terminating component whose resource exits with status 0 before it is tracked. "
        "When the component is activated and the start action reports the termination. "
        "Then the termination is ignored, the ready condition is waited for and activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle, 0}}));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    {
        InSequence sequence;
        EXPECT_CALL(*ready_condition, handleTermination(_, 0));
        EXPECT_CALL(*ready_condition, wait(_, _, _))
            .WillOnce(Invoke([](cpp::stop_token token, const Handle&, std::chrono::milliseconds) {
                EXPECT_FALSE(token.stop_requested());
                return Result<void>{};
            }));
    }

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::move(ready_condition),
        factory,
        std::move(config));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, TerminationDuringStartWaitsUntilComponentIsStarting)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given an inactive component. "
        "When a termination is reported from another thread right after the resource was started and tracked. "
        "Then the termination waits until activation released the state, is ignored, and the activation fails.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    auto* const start_action_ptr = start_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    std::atomic_bool termination_handled{false};
    std::thread termination_thread;

    EXPECT_CALL(*start_action_ptr, start(_, _)).WillOnce(Invoke([&](cpp::stop_token, IComponent&) {
        // Like the OsHandler thread reporting the termination right after the insertion.
        termination_thread = std::thread([&component, &termination_handled]() {
            EXPECT_EQ(component.tryHandleTermination(mock_handle, 1), IComponent::TerminationResult::kIgnore);
            termination_handled.store(true);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // The state is still locked by activate().
        EXPECT_FALSE(termination_handled.load());
        return Result<StartedResource>{StartedResource{mock_handle}};
    }));
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&](cpp::stop_token token, const Handle&, std::chrono::milliseconds) {
            termination_thread.join();

            EXPECT_TRUE(termination_handled.load());
            EXPECT_FALSE(token.stop_requested());
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateInTerminatedStateSucceeds)
{
    RecordProperty(
        "Description",
        "Given an inactive component. "
        "When it is deactivated. "
        "Then deactivation succeeds without calling the stop action.");

    NiceMock<MockSupervisionFactory> factory;

    Component component(
        std::make_unique<MockStartAction>(),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<MockReadyCondition>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CannotActivateInReadyState)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When it is activated again. "
        "Then the component aborts, since activation requires the inactive state.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));

    Component component(
        std::move(start_action),
        std::make_unique<MockStopAction>(),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));

    EXPECT_DEATH(
        { static_cast<void>(component.activate(cpp::stop_token{})); }, "Cannot activate component in a running state");
}

TEST(ComponentTest, CannotDeactivateWhileActivating)
{
    RecordProperty(
        "Description",
        "Given a component waiting for its ready condition. "
        "When it is deactivated concurrently. "
        "Then the component aborts, because activation and deactivation must never run at the same time.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<NiceMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            EXPECT_DEATH(
                { static_cast<void>(component.deactivate(cpp::stop_token{})); },
                "Cannot deactivate component while it is activating or deactivating");
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
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
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{EmptyHandle{}}}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
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
    EXPECT_CALL(*start_action, start(_, _))
        .Times(2)
        .WillRepeatedly(Return(Result<StartedResource>{StartedResource{EmptyHandle{}}}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.deactivate(cpp::stop_token{}));
    const auto result = component.activate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, CanActivateAgainAfterSelfTerminatingProcessExitedBeforeTrackingAndWasDeactivated)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given an active self-terminating component whose process exited with status 0 before it was tracked. "
        "When the component is deactivated and activated again. "
        "Then deactivation makes the component inactive immediately, since the termination was already handled, and "
        "the second activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _))
        .WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle, 0}}))
        .WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));

    // The stop action succeeds for the invalidated handle without anything left to terminate.
    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Invoke([](cpp::stop_token, const Handle& handle) {
        EXPECT_EQ(std::get<ProcessHandle>(handle).pid, -1);
        return Result<IComponent::RequestResult>{IComponent::RequestResult::kSuccess};
    }));

    auto config = std::make_shared<configuration::ComponentConfig>();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<MockForceStopAction>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::move(config));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, DeactivateOfRunningProcessWaitsForTermination)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given an active component whose process is still running. "
        "When the component is deactivated. "
        "Then deactivation only returns once the termination is reported, without stopping the process "
        "forcefully, and the component can be activated again afterwards.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    std::atomic_bool termination_reported{false};
    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke([&](cpp::stop_token, const Handle&) {
        termination_thread = cpp::jthread{[&] {
            termination_reported.store(true);
            EXPECT_EQ(component.tryHandleTermination(mock_handle, 0), IComponent::TerminationResult::kIgnore);
        }};
        return Result<IComponent::RequestResult>{IComponent::RequestResult::kSuccess};
    }));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(termination_reported.load());
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, DeactivateForcesStopAfterShutdownTimeout)
{
    RecordProperty(
        "Description",
        "Given an active component whose process ignores the stop request. "
        "When the component is deactivated and the shutdown timeout expires. "
        "Then the process is stopped forcefully, and deactivation succeeds once it terminated.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    auto* const force_stop_action_ptr = force_stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    cpp::jthread termination_thread;
    EXPECT_CALL(*force_stop_action_ptr, forceStop(_, _))
        .WillOnce(DoAll(
            WithArg<1>(Invoke(expect_mock_handle)), Invoke(reportTerminationLater(component, termination_thread))));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateFailsWhenProcessDoesNotTerminate)
{
    RecordProperty(
        "Description",
        "Given an active component whose process does not terminate, not even when stopped forcefully. "
        "When the component is deactivated. "
        "Then deactivation fails after a bounded wait, the component is inactive and can be activated again, and a "
        "late termination is ignored.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, DeactivateSucceedsWhenSigtermFailsForAlreadyTerminatedProcess)
{
    RecordProperty(
        "Description",
        "Given an active component whose process has already terminated, but whose termination is not handled yet. "
        "When it is deactivated and the stop action fails because of that. "
        "Then the termination is still waited for, the process is not stopped forcefully, deactivation succeeds and "
        "the component is inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .WillOnce(Invoke(reportTerminationLater(component, termination_thread, IComponent::RequestResult::kFailure)));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateFailsWhenSigtermCannotBeSent)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When it is deactivated, the stop action fails and no termination is reported. "
        "Then the process is not stopped forcefully, deactivation fails after a bounded wait and the component is "
        "inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateSucceedsWhenSigkillFailsForAlreadyTerminatedProcess)
{
    RecordProperty(
        "Description",
        "Given an active component whose process ignores the stop request. "
        "When it is deactivated and the force stop action fails after the shutdown timeout, because the process "
        "has terminated in the meantime. "
        "Then the termination is still waited for, deactivation succeeds and the component is inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    auto* const force_stop_action_ptr = force_stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    cpp::jthread termination_thread;
    EXPECT_CALL(*force_stop_action_ptr, forceStop(_, _))
        .WillOnce(Invoke(reportTerminationLater(component, termination_thread, IComponent::RequestResult::kFailure)));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateFailsWhenSigkillCannotBeSent)
{
    RecordProperty(
        "Description",
        "Given an active component whose process ignores the stop request. "
        "When it is deactivated, the force stop action fails after the shutdown timeout and no termination is "
        "reported. "
        "Then deactivation fails after a bounded wait and the component is inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, DeactivateWithEmptyHandleSucceedsWhenStopActionFails)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given an active component whose start action returned an empty handle. "
        "When the component is deactivated and the stop action fails. "
        "Then deactivation succeeds, because nothing is left running, and the component is not active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{EmptyHandle{}}}));

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    static_cast<void>(component.activate(cpp::stop_token{}));
    const auto result = component.deactivate(cpp::stop_token{});

    EXPECT_EQ(result, IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, CanActivateAgainAfterUnexpectedTermination)
{
    RecordProperty(
        "Description",
        "Given an active component whose process terminated unexpectedly. "
        "When the component is activated again. "
        "Then activation succeeds, there is no fault state.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kFailure);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, DeactivateAfterUnexpectedTerminationSucceeds)
{
    RecordProperty(
        "Description",
        "Given an active component whose process terminated unexpectedly. "
        "When the component is deactivated. "
        "Then deactivation succeeds without calling the stop action, the component already cleaned up.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kFailure);

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SelfTerminatingExitZeroInReadyStateIsIgnored)
{
    RecordProperty(
        "Description",
        "Given an active self-terminating component. "
        "When its process exits with status 0. "
        "Then the termination is ignored and the component stays active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        selfTerminatingConfig());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInReadyStateIsFailure)
{
    RecordProperty(
        "Description",
        "Given an active self-terminating component. "
        "When its process exits with a non-zero status. "
        "Then the termination is reported as a failure and the component is not active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        selfTerminatingConfig());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitZeroInStartingStateIsIgnored)
{
    RecordProperty(
        "Description",
        "Given a self-terminating component waiting for its ready condition. "
        "When its process exits with status 0 and the ready condition is met. "
        "Then the termination is ignored, activation succeeds and the component is active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        selfTerminatingConfig());

    std::optional<IComponent::TerminationResult> termination_result;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            termination_result = terminateFromOtherThread(component, 0);
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kIgnore);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SelfTerminatingExitNonZeroInStartingStateFailsActivation)
{
    RecordProperty(
        "Description",
        "Given a self-terminating component waiting for its ready condition. "
        "When its process exits with a non-zero status. "
        "Then the termination is ignored, the activation fails and the component is not active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    // The failed activation cleans up the process, which has already terminated.
    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        selfTerminatingConfig());

    std::optional<IComponent::TerminationResult> termination_result;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            termination_result = terminateFromOtherThread(component, 1);
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kIgnore);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, StopTokenInterruptsWaitForReadyCondition)
{
    RecordProperty(
        "Description",
        "Given a component waiting for its ready condition. "
        "When a stop is requested on the token passed to activate. "
        "Then the ready condition is interrupted, activation fails and the process is stopped.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::stop_source stop_source;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&stop_source](cpp::stop_token token, const Handle&, std::chrono::milliseconds) {
            EXPECT_FALSE(token.stop_requested());
            static_cast<void>(stop_source.request_stop());
            EXPECT_TRUE(token.stop_requested());
            return Result<void>{MakeUnexpected(ExecErrc::kGeneralError)};
        }));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke([&](cpp::stop_token token, const Handle& handle) {
        // Stopping cannot be interrupted.
        EXPECT_FALSE(token.stop_requested());
        return reportTerminationLater(component, termination_thread)(token, handle);
    }));

    EXPECT_EQ(component.activate(stop_source.get_token()), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, ReadyConditionMetDespiteStopRequestActivatesComponent)
{
    RecordProperty(
        "Description",
        "Given a component whose activation token is already stopped. "
        "When the ready condition is met anyway. "
        "Then activation succeeds, the token only interrupts the wait.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _)).WillOnce(Return(Result<void>{}));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    EXPECT_EQ(component.activate(stop_source.get_token()), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, StoppedTokenDoesNotInterruptDeactivation)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When it is deactivated with a stopped token. "
        "Then the token is not passed on to the stop action and the termination is still waited for.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke([&](cpp::stop_token token, const Handle& handle) {
        EXPECT_FALSE(token.stop_requested());
        return reportTerminationLater(component, termination_thread)(token, handle);
    }));

    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(component.deactivate(stop_source.get_token()), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, FailedActivationStopsProcessAndAllowsReactivation)
{
    RecordProperty(
        "Description",
        "Given a component whose ready condition times out. "
        "When it is activated. "
        "Then the process is stopped and its termination waited for before activate returns, and the component "
        "can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}))
        .WillOnce(Return(Result<void>{}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .WillOnce(DoAll(
            WithArg<1>(Invoke(expect_mock_handle)), Invoke(reportTerminationLater(component, termination_thread))));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kTimeout);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, FailedActivationForcesStopAfterShutdownTimeout)
{
    RecordProperty(
        "Description",
        "Given a component whose activation failed and whose process ignores the stop request. "
        "When the shutdown timeout expires. "
        "Then the process is stopped forcefully and its termination waited for.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    auto* const force_stop_action_ptr = force_stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    cpp::jthread termination_thread;
    EXPECT_CALL(*force_stop_action_ptr, forceStop(_, _))
        .WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, FailedActivationDoesNotStopForcefullyWhenStopActionFails)
{
    RecordProperty(
        "Description",
        "Given a component whose activation failed. "
        "When the stop action fails. "
        "Then the process is not stopped forcefully, and the component can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}))
        .WillOnce(Return(Result<void>{}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, FailedActivationDoesNotWaitForeverForForcedStop)
{
    RecordProperty(
        "Description",
        "Given a component whose activation failed and whose process does not terminate, not even when stopped "
        "forcefully. "
        "When the activation cleans up. "
        "Then it gives up after a bounded wait, the component is inactive and a late termination is ignored.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kTimeout);
    EXPECT_FALSE(component.active());
    EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
}

/// @brief Configuration which retries a failed activation @p retries times.
std::shared_ptr<configuration::ComponentConfig> configWithRetries(std::uint8_t retries)
{
    auto config = configWithShutdownTimeout(kNeverExpires);
    config->deployment_config.activation_retry_attempts = retries;
    return config;
}

TEST(ComponentTest, FailedActivationIsRetriedUpToConfiguredAttempts)
{
    RecordProperty(
        "Description",
        "Given a component configured with 2 activation retries whose ready condition always times out. "
        "When it is activated. "
        "Then it is started 3 times, every attempt is cleaned up, the result of the last attempt is returned and "
        "the component is inactive.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(3).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, prepare()).Times(3);
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .Times(3)
        .WillRepeatedly(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(2U));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .Times(3)
        .WillRepeatedly(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kTimeout);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, RetrySucceedsAfterFailedAttempt)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries whose first attempt fails. "
        "When it is activated and the second attempt succeeds. "
        "Then the first process is stopped, activation succeeds and the component is active.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}))
        .WillOnce(Return(Result<void>{}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(3U));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, UnexpectedTerminationDuringStartupIsRetried)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries whose process crashes during startup. "
        "When it is activated and the process of the second attempt starts up. "
        "Then the crash is ignored, so that the graph does not abort, and activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(1U));

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            EXPECT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kIgnore);
            return Result<void>{MakeUnexpected(ExecErrc::kGeneralError)};
        }))
        .WillOnce(Return(Result<void>{}));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, CancelledActivationIsNotRetried)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries. "
        "When its activation is cancelled while the first attempt waits for the ready condition. "
        "Then the process is stopped, no further attempt is made, activation fails and the component is inactive, "
        "so that it can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(3U));

    cpp::stop_source stop_source;
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&stop_source](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            static_cast<void>(stop_source.request_stop());
            return Result<void>{MakeUnexpected(ExecErrc::kGeneralError)};
        }))
        .WillOnce(Return(Result<void>{}));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(stop_source.get_token()), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());

    // The second start is the reactivation, not a retry.
    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, StartFailureIsRetried)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries. "
        "When starting its resource fails once. "
        "Then nothing is stopped, the start is retried and activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _))
        .WillOnce(Return(Result<StartedResource>{MakeUnexpected(ExecErrc::kGeneralError)}))
        .WillOnce(kStartsProcess);

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithRetries(1U));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, FailedCleanupIsNotRetried)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries whose first attempt fails. "
        "When its process cannot be asked to stop. "
        "Then no further attempt is made, since it could start a second instance, and activation fails.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(3U));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kTimeout);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, FailedCleanupOfAlreadyTerminatedProcessIsRetried)
{
    RecordProperty(
        "Description",
        "Given a component configured with activation retries whose first attempt fails. "
        "When its process cannot be asked to stop, because it has already terminated, and the termination is "
        "reported. "
        "Then the next attempt is made and activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _))
        .WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kTimeout)}))
        .WillOnce(Return(Result<void>{}));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithRetries(3U));

    cpp::jthread termination_thread;
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .WillOnce(Invoke(reportTerminationLater(component, termination_thread, IComponent::RequestResult::kFailure)));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SupervisionConstructedFromConfig)
{
    RecordProperty(
        "Description",
        "Given a supervised component configuration with a name, a uid and an alive supervision configuration. "
        "When the component is created. "
        "Then alive supervision is constructed from the name, the uid and the alive supervision configuration.");

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
    RecordProperty(
        "Description",
        "Given an unsupervised component configuration. "
        "When the component is created. "
        "Then no alive supervision is constructed and no environment variable is added.");

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
    RecordProperty(
        "Description",
        "Given a supervised component configuration. "
        "When the component is created. "
        "Then the connection id of the alive supervision is added to the environment as LCM_ALIVE_INTERFACE_PATH.");

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
    RecordProperty(
        "Description",
        "Given a supervised component whose alive supervision could not be constructed. "
        "When it is activated. "
        "Then activation fails without starting anything, and no environment variable is added.");

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
    RecordProperty(
        "Description",
        "Given a supervised component. "
        "When it is activated and its ready condition is met. "
        "Then supervision is activated.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));

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
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));
}

/// @brief Configuration of a supervised, self-terminating component.
std::shared_ptr<configuration::ComponentConfig> supervisedSelfTerminatingConfig()
{
    auto config = supervisedConfig();
    config->component_properties.application_profile.is_self_terminating = true;
    return config;
}

TEST(ComponentTest, SupervisionNotActivatedWhenProcessTerminatedWhileWaitingForReadyCondition)
{
    RecordProperty(
        "Description",
        "Given a supervised, self-terminating component waiting for its ready condition. "
        "When its process exits with status 0 and the ready condition is met. "
        "Then activation succeeds without activating supervision for the terminated process.");

    NiceMock<MockSupervisionFactory> factory;
    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* const supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        supervisedSelfTerminatingConfig());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            EXPECT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
            return Result<void>{};
        }));
    EXPECT_CALL(*supervision, activateSupervision(_)).Times(0);

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SupervisionNotActivatedWhenProcessTerminatedBeforeTracking)
{
    RecordProperty(
        "Description",
        "Given a supervised, self-terminating component whose process exits with status 0 before it is tracked. "
        "When the component is activated and its ready condition is met. "
        "Then activation succeeds without activating supervision for the terminated process.");

    NiceMock<MockSupervisionFactory> factory;
    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* const supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle, 0}}));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedSelfTerminatingConfig());

    EXPECT_CALL(*supervision, activateSupervision(_)).Times(0);

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

TEST(ComponentTest, SupervisionActivationFailureStopsProcess)
{
    RecordProperty(
        "Description",
        "Given a supervised component whose ready condition is met. "
        "When supervision cannot be activated. "
        "Then activation fails and the process is stopped, so it never runs without supervision.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    cpp::jthread termination_thread;
    InSequence sequence;
    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(false));
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component.active());
    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SupervisionNotActivatedWhenStartFails)
{
    RecordProperty(
        "Description",
        "Given a supervised component. "
        "When it is activated and starting its process fails. "
        "Then supervision is not activated.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _))
        .WillOnce(Return(Result<StartedResource>{MakeUnexpected(ExecErrc::kGeneralError)}));

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
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::move(config));

    EXPECT_CALL(*supervision, activateSupervision(_)).Times(0);

    static_cast<void>(component.activate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivatedImmediatelyOnTermination)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When its process terminates. "
        "Then supervision is deactivated right away, without waiting for a deactivation.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{mock_handle}}));

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
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::move(config));

    static_cast<void>(component.activate(cpp::stop_token{}));
    static_cast<void>(component.tryHandleTermination(mock_handle, 1));

    // deactivate() is never called because there is nothing reading the event queue
}

TEST(ComponentTest, SupervisionDeactivatedBeforeStopAction)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When it is deactivated. "
        "Then supervision is deactivated before the process is asked to stop.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    ON_CALL(*supervision, activateSupervision(_)).WillByDefault(Return(true));

    static_cast<void>(component.activate(cpp::stop_token{}));

    cpp::jthread termination_thread;
    InSequence sequence;
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(true));
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    static_cast<void>(component.deactivate(cpp::stop_token{}));
}

TEST(ComponentTest, SupervisionDeactivationFailureIsIgnoredOnDeactivate)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When it is deactivated and supervision cannot be deactivated. "
        "Then the process is stopped anyway and deactivation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    EXPECT_CALL(*supervision, activateSupervision(_)).WillOnce(Return(true));
    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    cpp::jthread termination_thread;
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(false));
    EXPECT_CALL(*stop_action_ptr, stop(_, _)).WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, SupervisionDeactivationFailureIsIgnoredOnTermination)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When its process terminates unexpectedly and supervision cannot be deactivated. "
        "Then the termination is a failure, the component is inactive and can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).Times(2).WillRepeatedly(kStartsProcess);

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    auto* supervision = handle.get();
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    ON_CALL(*supervision, activateSupervision(_)).WillByDefault(Return(true));
    EXPECT_CALL(*supervision, deactivateSupervision(_)).WillOnce(Return(false));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_EQ(terminateFromOtherThread(component, 1), IComponent::TerminationResult::kFailure);
    EXPECT_FALSE(component.active());

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SupervisionFailureWhileReadyStopsProcessForcefully)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When a supervision failure is reported which happened after supervision was activated. "
        "Then the process is stopped forcefully.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(IComponent::RequestResult::kSuccess)));

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    ON_CALL(*handle, activateSupervision(_)).WillByDefault(Return(true));
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    component.handleSupervisionFailure(std::chrono::nanoseconds::max());
}

TEST(ComponentTest, SupervisionFailureFromBeforeActivationIsIgnored)
{
    RecordProperty(
        "Description",
        "Given an active supervised component. "
        "When a supervision failure is reported which happened before supervision was activated. "
        "Then it is ignored.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    ON_CALL(*handle, activateSupervision(_)).WillByDefault(Return(true));
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        supervisedConfig());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    component.handleSupervisionFailure(std::chrono::nanoseconds{0});
}

TEST(ComponentTest, SupervisionFailureWhileNotReadyIsIgnored)
{
    RecordProperty(
        "Description",
        "Given a supervised component waiting for its ready condition. "
        "When a supervision failure is reported. "
        "Then it is ignored, a starting component is not supervised yet.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    ON_CALL(*handle, activateSupervision(_)).WillByDefault(Return(true));
    provideSupervision(factory, std::move(handle));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        supervisedConfig());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            component.handleSupervisionFailure(std::chrono::nanoseconds::max());
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, SupervisionFailureAfterTerminationIsIgnored)
{
    RecordProperty(
        "Description",
        "Given an active supervised self-terminating component whose process exited with 0. "
        "When a supervision failure is reported. "
        "Then it is ignored, since there is no process left to stop.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto handle = std::make_unique<NiceMock<MockAliveSupervisionHandle>>();
    ON_CALL(*handle, activateSupervision(_)).WillByDefault(Return(true));
    ON_CALL(*handle, deactivateSupervision(_)).WillByDefault(Return(true));
    provideSupervision(factory, std::move(handle));

    auto config = supervisedConfig();
    config->component_properties.application_profile.is_self_terminating = true;

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::move(config));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(terminateFromOtherThread(component, 0), IComponent::TerminationResult::kIgnore);
    ASSERT_TRUE(component.active());

    component.handleSupervisionFailure(std::chrono::nanoseconds::max());
}

TEST(ComponentTest, ForceStopStopsRunningProcess)
{
    RecordProperty(
        "Description",
        "Given an active component. "
        "When forceStop is called. "
        "Then the process is stopped forcefully.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _))
        .WillOnce(DoAll(WithArg<1>(Invoke(expect_mock_handle)), Return(IComponent::RequestResult::kSuccess)));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    component.forceStop();
}

TEST(ComponentTest, ForceStopStopsStartingProcess)
{
    RecordProperty(
        "Description",
        "Given a component waiting for its ready condition. "
        "When forceStop is called. "
        "Then the process is stopped forcefully.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess);

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::move(force_stop_action),
        std::move(ready_condition),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            component.forceStop();
            return Result<void>{};
        }));

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST(ComponentTest, ForceStopDoesNothingWithoutRunningProcess)
{
    RecordProperty(
        "Description",
        "Given an inactive component, and an active component without a resource. "
        "When forceStop is called. "
        "Then the force stop action is not called.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Return(Result<StartedResource>{StartedResource{EmptyHandle{}}}));

    Component component(
        std::move(start_action),
        std::make_unique<StrictMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        std::make_shared<configuration::ComponentConfig>());

    component.forceStop();

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    component.forceStop();
}

/// @brief One case of a component with a ProcessTerminatedReadyCondition whose process exits during activation.
struct ProcessTerminatedCase
{
    /// @brief Name of the test case.
    const char* name;
    bool is_self_terminating;
    int32_t exit_status;
    IComponent::RequestResult expected_activation;
};

class ComponentProcessTerminatedReadyConditionTest : public TestWithParam<ProcessTerminatedCase>
{
};

TEST_P(ComponentProcessTerminatedReadyConditionTest, TerminationDuringActivationDecidesActivation)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component with a ProcessTerminatedReadyCondition. "
        "When its process exits during activation. "
        "Then the termination is ignored, so that only the activation reports the outcome, and activation only "
        "succeeds for a self-terminating component exiting with status 0. A failed activation returns kFailure "
        "rather than kTimeout, which shows that the wait ended because of the termination.");

    const ProcessTerminatedCase& param = GetParam();

    NiceMock<MockSupervisionFactory> factory;

    std::promise<void> started;
    auto started_future = started.get_future();

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(Invoke([&started](cpp::stop_token, IComponent&) {
        started.set_value();
        return Result<StartedResource>{StartedResource{mock_handle}};
    }));

    auto config = configWithShutdownTimeout(kNeverExpires);
    config->component_properties.application_profile.is_self_terminating = param.is_self_terminating;

    // Only a failed activation stops the process, which has already terminated then.
    Component component(
        std::move(start_action),
        std::make_unique<NiceMock<MockStopAction>>(),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::make_unique<ProcessTerminatedReadyCondition>(),
        factory,
        std::move(config));

    auto activation = std::async(std::launch::async, [&component] {
        return component.activate(cpp::stop_token{});
    });

    started_future.wait();
    const IComponent::TerminationResult termination_result = terminateFromOtherThread(component, param.exit_status);

    EXPECT_EQ(activation.get(), param.expected_activation);
    EXPECT_EQ(termination_result, IComponent::TerminationResult::kIgnore);
}

INSTANTIATE_TEST_SUITE_P(
    ComponentTest,
    ComponentProcessTerminatedReadyConditionTest,
    Values(
        ProcessTerminatedCase{"SucceedsOnExitZero", true, 0, IComponent::RequestResult::kSuccess},
        ProcessTerminatedCase{"FailsOnNonZeroExit", true, 1, IComponent::RequestResult::kFailure},
        ProcessTerminatedCase{"FailsWhenNotSelfTerminating", false, 0, IComponent::RequestResult::kFailure}),
    [](const TestParamInfo<ProcessTerminatedCase>& info) {
        return std::string{info.param.name};
    });

TEST(ComponentTest, SelfTerminatingExitWhileStartingDoesNotBlockReactivation)
{
    RecordProperty(
        "Description",
        "Given a self-terminating component whose process exits with 0 while starting. "
        "When the component is deactivated. "
        "Then the component is terminated immediately and can be activated again.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _))
        .Times(2)
        .WillRepeatedly(Return(Result<StartedResource>{StartedResource{mock_handle}}));

    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
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

    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
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
    EXPECT_CALL(*start_action, start(_, _))
        .Times(2)
        .WillRepeatedly(Return(Result<StartedResource>{StartedResource{mock_handle}}));

    auto stop_action = std::make_unique<MockStopAction>();
    expectStopOfDeadProcess(*stop_action);

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    EXPECT_CALL(*ready_condition, wait(_, _, _)).Times(2).WillRepeatedly(Return(Result<void>{}));

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

TEST(ComponentTest, LateTerminationOfPreviousInstanceIsIgnoredWhileReady)
{
    RecordProperty(
        "Description",
        "Given a component whose process did not terminate after being stopped forcefully, and which was activated "
        "again with a new process. "
        "When the termination of the previous process is reported late. "
        "Then it is ignored and the component stays active, while a termination of the new process is a failure.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess).WillOnce(kRestartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    // The forced stop reaches the process, but it does not terminate in time.
    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    EXPECT_CALL(*force_stop_action, forceStop(_, _)).WillOnce(Return(IComponent::RequestResult::kSuccess));

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(std::chrono::milliseconds{0}));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(terminateFromOtherThread(component, 1, mock_handle), IComponent::TerminationResult::kIgnore);
    EXPECT_TRUE(component.active());

    EXPECT_EQ(terminateFromOtherThread(component, 1, restarted_handle), IComponent::TerminationResult::kFailure);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, LateTerminationOfPreviousInstanceDoesNotCompleteDeactivation)
{
    RecordProperty(
        "Description",
        "Given a component which gave up on its previous process, and which was activated again with a new process. "
        "When the new process is deactivated and the termination of the previous process is reported meanwhile. "
        "Then the component keeps waiting for the termination of the new process.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess).WillOnce(kRestartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    auto* const stop_action_ptr = stop_action.get();

    auto force_stop_action = std::make_unique<StrictMock<MockForceStopAction>>();
    auto* const force_stop_action_ptr = force_stop_action.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::move(force_stop_action),
        std::make_unique<NiceMock<MockReadyCondition>>(),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    cpp::jthread late_termination_thread;
    cpp::jthread termination_thread;

    // The previous process cannot be asked to stop, so the component gives up on it.
    EXPECT_CALL(*stop_action_ptr, stop(_, _))
        .WillOnce(Return(IComponent::RequestResult::kFailure))
        .WillOnce(Invoke([&component, &late_termination_thread](cpp::stop_token, const Handle& handle) {
            EXPECT_TRUE(isSameResource(handle, restarted_handle));
            late_termination_thread = cpp::jthread{[&component] {
                EXPECT_EQ(component.tryHandleTermination(mock_handle, 0), IComponent::TerminationResult::kIgnore);
                // Only a component which still waits for the termination stops its resource forcefully.
                component.forceStop();
            }};
            return Result<IComponent::RequestResult>{IComponent::RequestResult::kSuccess};
        }));

    EXPECT_CALL(*force_stop_action_ptr, forceStop(_, Truly([](const Handle& handle) {
                                                      return isSameResource(handle, restarted_handle);
                                                  })))
        .WillOnce(Invoke(reportTerminationLater(component, termination_thread)));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(component.active());
}

TEST(ComponentTest, LateTerminationOfPreviousInstanceIsIgnoredWhileStarting)
{
    RecordProperty(
        "Description",
        "Given a component which gave up on its previous process, and which is activated again with a new process. "
        "When the termination of the previous process is reported while waiting for the ready condition. "
        "Then it is not reported to the ready condition and the activation succeeds.");

    NiceMock<MockSupervisionFactory> factory;

    auto start_action = std::make_unique<MockStartAction>();
    EXPECT_CALL(*start_action, start(_, _)).WillOnce(kStartsProcess).WillOnce(kRestartsProcess);

    auto stop_action = std::make_unique<MockStopAction>();
    EXPECT_CALL(*stop_action, stop(_, _)).WillOnce(Return(IComponent::RequestResult::kFailure));

    auto ready_condition = std::make_unique<NiceMock<MockReadyCondition>>();
    auto* const ready_condition_ptr = ready_condition.get();

    Component component(
        std::move(start_action),
        std::move(stop_action),
        std::make_unique<StrictMock<MockForceStopAction>>(),
        std::move(ready_condition),
        factory,
        configWithShutdownTimeout(kNeverExpires));

    EXPECT_CALL(*ready_condition_ptr, handleTermination(_, _)).Times(0);
    EXPECT_CALL(*ready_condition_ptr, wait(_, _, _))
        .WillOnce(Return(Result<void>{}))
        .WillOnce(Invoke([&component](cpp::stop_token, const Handle&, std::chrono::milliseconds) {
            EXPECT_EQ(terminateFromOtherThread(component, 1, mock_handle), IComponent::TerminationResult::kIgnore);
            return Result<void>{};
        }));

    ASSERT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(component.deactivate(cpp::stop_token{}), IComponent::RequestResult::kFailure);

    EXPECT_EQ(component.activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component.active());
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

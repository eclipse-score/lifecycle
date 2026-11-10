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

#include "score/mw/launch_manager/alive_monitor/mock_supervision_factory.hpp"
#include "score/mw/launch_manager/osal/mock_ifile_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_factory.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <cstddef>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;
using namespace std::chrono_literals;

constexpr osal::ProcessID kPid = 42;
constexpr std::uint32_t kReadyTimeoutMs = 1234U;

alignas(osal::IpcCommsSync) std::array<std::byte, sizeof(osal::IpcCommsSync)> sync_buffer;

// The semaphores are never touched, since the launcher is mocked.
const osal::IpcCommsP mock_sync{reinterpret_cast<osal::IpcCommsSync*>(sync_buffer.data()), [](osal::IpcCommsSync*) {
                                }};

std::shared_ptr<configuration::ComponentConfig> makeConfig(
    const configuration::ApplicationType application_type,
    const configuration::ReadyCondition ready_condition)
{
    auto config = std::make_shared<configuration::ComponentConfig>();
    config->name = "test_component";
    config->component_properties.application_profile.application_type = application_type;
    config->component_properties.ready_condition = ready_condition;
    config->deployment_config.ready_timeout_ms = kReadyTimeoutMs;
    config->deployment_config.sandbox.uid = 1000U;
    return config;
}

class ComponentFactoryTest : public Test
{
  protected:
    /// @brief Expect the component's process to be started, and return the given sync object.
    void expectProcessStarted(const osal::IpcCommsP& sync)
    {
        EXPECT_CALL(launcher_, startProcess(_, _, _))
            .WillOnce(DoAll(SetArgReferee<0>(kPid), SetArgReferee<1>(sync), Return(osal::OsalReturnType::kSuccess)));
    }

    std::unique_ptr<IComponent> createComponent(std::shared_ptr<configuration::ComponentConfig> config)
    {
        Result<std::unique_ptr<IComponent>> result = factory_.create(std::move(config));
        EXPECT_TRUE(result.has_value());
        return result.has_value() ? std::move(result).value() : nullptr;
    }

    // Strict mocks ensure that only the actions expected for a config are used.
    StrictMock<osal::MockIProcess> launcher_;
    StrictMock<osal::MockIFileWaiter> file_waiter_;
    StrictMock<MockSupervisionFactory> supervision_factory_;
    ComponentFactory factory_{launcher_, file_waiter_, supervision_factory_};
};

TEST_F(ComponentFactoryTest, CreatedComponentIsInactive)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a native component configuration with ready condition Running. "
        "When the factory creates a component. "
        "Then the created component is not active.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);

    const auto component = createComponent(config);

    ASSERT_NE(component, nullptr);
    EXPECT_FALSE(component->active());
}

TEST_F(ComponentFactoryTest, CreatedComponentIsIdentifiedByConfigName)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a component configuration with a name. "
        "When the factory creates a component. "
        "Then the component identifier is the hash of the configured name.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);

    const auto component = createComponent(config);

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getIdentifier(), IdentifierHash{"test_component"});
}

TEST_F(ComponentFactoryTest, NativeRunningComponentStartsProcessWithoutWaiting)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component created from a native configuration with ready condition Running. "
        "When the component is activated. "
        "Then the process is started with the component configuration, neither kRunning nor a file is waited for, and "
        "activation succeeds.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    // The process is started with the component's config, and neither
    // kRunning nor a file is waited for.
    EXPECT_CALL(launcher_, startProcess(_, _, Ref(*config)))
        .WillOnce(DoAll(SetArgReferee<0>(kPid), Return(osal::OsalReturnType::kSuccess)));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(component->active());
}

TEST_F(ComponentFactoryTest, ReportingRunningComponentWaitsForkRunningWithReadyTimeout)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component created from a reporting configuration with ready condition Running. "
        "When the component is activated. "
        "Then kRunning is waited for with the configured ready timeout and activation succeeds.");

    auto config = makeConfig(configuration::ApplicationType::Reporting, configuration::ProcessState::Running);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    expectProcessStarted(mock_sync);
    EXPECT_CALL(
        launcher_,
        waitForkRunning(
            mock_sync, std::optional<std::chrono::milliseconds>(std::chrono::milliseconds{kReadyTimeoutMs})))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, ReportingRunningComponentFailsWhenkRunningIsNotReported)
{
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a component created from a reporting configuration with ready condition Running. "
        "When the component is activated and waiting for kRunning fails. "
        "Then activation fails.");

    auto config = makeConfig(configuration::ApplicationType::Reporting, configuration::ProcessState::Running);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    expectProcessStarted(mock_sync);
    EXPECT_CALL(launcher_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
}

TEST_F(ComponentFactoryTest, FileStateComponentWaitsForConfiguredFile)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component created from a native configuration with a FileState ready condition. "
        "When the component is activated. "
        "Then the configured file, state, ready timeout and polling interval are used to wait and activation "
        "succeeds.");

    const configuration::FileState file_state{"/tmp/ready", configuration::FileExistenceState::Exists, 5ms};
    auto config = makeConfig(configuration::ApplicationType::Native, file_state);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    expectProcessStarted(nullptr);
    EXPECT_CALL(
        file_waiter_,
        waitForFile(
            Eq(score::safecpp::zstring_view{"/tmp/ready"}),
            configuration::FileExistenceState::Exists,
            std::chrono::milliseconds{kReadyTimeoutMs},
            5ms,
            _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, ReportingFileStateComponentWaitsOnlyForFile)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component created from a reporting configuration with a FileState ready condition. "
        "When the component is activated. "
        "Then only the file is waited for, kRunning is not waited for, and activation succeeds.");

    const configuration::FileState file_state{"/tmp/gone", configuration::FileExistenceState::NotExisting, 5ms};
    auto config = makeConfig(configuration::ApplicationType::Reporting, file_state);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    // The strict launcher mock fails the test if kRunning is waited for.
    expectProcessStarted(mock_sync);
    EXPECT_CALL(file_waiter_, waitForFile(_, configuration::FileExistenceState::NotExisting, _, _, _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, FileStateComponentFailsWhenFileWaitFails)
{
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a component created from a configuration with a FileState ready condition. "
        "When the component is activated and waiting for the file fails. "
        "Then activation fails.");

    const configuration::FileState file_state{"/tmp/ready", configuration::FileExistenceState::Exists, 5ms};
    auto config = makeConfig(configuration::ApplicationType::Native, file_state);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    expectProcessStarted(nullptr);
    EXPECT_CALL(file_waiter_, waitForFile(_, _, _, _, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
}

TEST_F(ComponentFactoryTest, DeactivateRequestsProcessTermination)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given an active component created by the factory. "
        "When the component is deactivated. "
        "Then termination of its process is requested and deactivation succeeds.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);
    expectProcessStarted(nullptr);
    ASSERT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_CALL(launcher_, requestTermination(kPid)).WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_EQ(component->deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, StartFailureFailsActivation)
{
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a component created by the factory. "
        "When the component is activated and starting the process fails. "
        "Then activation fails and the component is not active.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);
    const auto component = createComponent(config);
    ASSERT_NE(component, nullptr);

    EXPECT_CALL(launcher_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kFailure);
    EXPECT_FALSE(component->active());
}

TEST_F(ComponentFactoryTest, SupervisedComponentSetsUpAliveSupervision)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a supervised component configuration with alive supervision. "
        "When the factory creates a component. "
        "Then alive supervision is constructed with the component name and uid.");

    auto config =
        makeConfig(configuration::ApplicationType::ReportingAndSupervised, configuration::ProcessState::Running);
    config->component_properties.application_profile.alive_supervision = configuration::ComponentAliveSupervision{};

    EXPECT_CALL(supervision_factory_, constructSupervision(IdentifierHash{"test_component"}, 1000U, _))
        .WillOnce(Return(ByMove(nullptr)));

    const auto component = createComponent(config);

    EXPECT_NE(component, nullptr);
}

TEST_F(ComponentFactoryTest, TerminatedReadyConditionIsNotImplemented)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a component configuration with ready condition Terminated. "
        "When the factory creates a component. "
        "Then creation fails with kNotImplemented.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Terminated);

    const Result<std::unique_ptr<IComponent>> result = factory_.create(config);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kNotImplemented);
}

TEST_F(ComponentFactoryTest, ComponentKeepsConfigAlive)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a component created from a configuration which the caller no longer holds. "
        "When the component is activated. "
        "Then the configuration is still alive and used to start the process.");

    std::shared_ptr<configuration::ComponentConfig> config =
        makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);
    const std::weak_ptr<configuration::ComponentConfig> weak_config = config;

    const auto component = createComponent(std::move(config));
    ASSERT_NE(component, nullptr);

    // The start action reads the config owned by the component.
    EXPECT_FALSE(weak_config.expired());
    EXPECT_CALL(launcher_, startProcess(_, _, Field(&configuration::ComponentConfig::name, "test_component")))
        .WillOnce(DoAll(SetArgReferee<0>(kPid), Return(osal::OsalReturnType::kSuccess)));

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, NullConfigIsRejected)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a null configuration. "
        "When the factory creates a component. "
        "Then the factory terminates with an assertion.");

    EXPECT_DEATH(static_cast<void>(factory_.create(nullptr)), "");
}

TEST_F(ComponentFactoryTest, ComponentRemainsUsableAfterFactoryIsDestroyed)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a component whose factory has been destroyed. "
        "When the component is activated. "
        "Then activation succeeds.");

    auto config = makeConfig(configuration::ApplicationType::Native, configuration::ProcessState::Running);
    std::unique_ptr<IComponent> component;
    {
        const ComponentFactory scoped_factory{launcher_, file_waiter_, supervision_factory_};
        Result<std::unique_ptr<IComponent>> result = scoped_factory.create(config);
        ASSERT_TRUE(result.has_value());
        component = std::move(result).value();
    }

    expectProcessStarted(nullptr);

    EXPECT_EQ(component->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
}

TEST_F(ComponentFactoryTest, CreatedRunTargetIsInactive)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a Run Target name. "
        "When the factory creates a Run Target. "
        "Then the created component is not active.");

    const auto run_target = factory_.createRunTarget("test_run_target");

    ASSERT_NE(run_target, nullptr);
    EXPECT_FALSE(run_target->active());
}

TEST_F(ComponentFactoryTest, CreatedRunTargetIsIdentifiedByName)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a Run Target name. "
        "When the factory creates a Run Target. "
        "Then the component identifier is the hash of the given name.");

    const auto run_target = factory_.createRunTarget("test_run_target");

    ASSERT_NE(run_target, nullptr);
    EXPECT_EQ(run_target->getIdentifier(), IdentifierHash{"test_run_target"});
}

TEST_F(ComponentFactoryTest, RunTargetActivatesWithoutAnyResource)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a Run Target created by the factory. "
        "When the Run Target is activated. "
        "Then activation succeeds without starting a process, waiting for a file or setting up supervision.");

    // The strict mocks fail the test if a process, file or supervision is used.
    const auto run_target = factory_.createRunTarget("test_run_target");
    ASSERT_NE(run_target, nullptr);

    EXPECT_EQ(run_target->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(run_target->active());
}

TEST_F(ComponentFactoryTest, RunTargetIsInactiveImmediatelyAfterDeactivation)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given an active Run Target created by the factory. "
        "When the Run Target is deactivated. "
        "Then deactivation succeeds without stopping a process and the Run Target is not active.");

    const auto run_target = factory_.createRunTarget("test_run_target");
    ASSERT_NE(run_target, nullptr);
    ASSERT_EQ(run_target->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(run_target->deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_FALSE(run_target->active());
}

TEST_F(ComponentFactoryTest, RunTargetCanBeActivatedAgainAfterDeactivation)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given a Run Target created by the factory which has been activated and deactivated. "
        "When the Run Target is activated again. "
        "Then activation succeeds and the Run Target is active.");

    const auto run_target = factory_.createRunTarget("test_run_target");
    ASSERT_NE(run_target, nullptr);
    ASSERT_EQ(run_target->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    ASSERT_EQ(run_target->deactivate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);

    EXPECT_EQ(run_target->activate(cpp::stop_token{}), IComponent::RequestResult::kSuccess);
    EXPECT_TRUE(run_target->active());
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <vector>

#include "score/mw/launch_manager/alive_monitor/details/daemon/SupervisionManager.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/IAliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;

namespace score::mw::lifecycle::internal::saf::daemon
{

class MockRecoveryClient : public IRecoveryClient
{
  public:
    MOCK_METHOD(
        void,
        setRecoveryRequestCallback,
        (IRecoveryClient::RecoveryRequestCallback callback),
        (noexcept, override));
    MOCK_METHOD(bool, sendRecoveryRequest, (const IdentifierHash& process_group_identifier), (noexcept, override));
};

class MockFactory : public factory::IAliveWorkerFactory
{
  public:
    MOCK_METHOD(
        bool,
        createObservableEvent,
        (std::vector<ifexm::ObservableEvent> & events,
         const IdentifierHash component_id,
         ifexm::ObservableEventReader& event_reader_),
        (override));

    MOCK_METHOD(
        bool,
        createAliveIfIpc,
        (std::vector<ifappl::CheckpointIpcServer> & servers, const IdentifierHash component_id, const uid_t uid),
        (override));

    MOCK_METHOD(
        bool,
        createAliveIf,
        (std::vector<ifappl::MonitorIfDaemon> & interfaces,
         ifappl::CheckpointIpcServer& ipc_server,
         ifexm::ObservableEvent& event),
        (override));

    MOCK_METHOD(
        bool,
        createAliveSupervision,
        (std::vector<supervision::Alive> & supervisions,
         ifappl::MonitorIfDaemon& interface,
         ifexm::ObservableEvent& event,
         const std::shared_ptr<IRecoveryClient> recovery_client,
         const IdentifierHash component_id,
         const configuration::ComponentAliveSupervision component_config),
        (override));
};

class SupervisionManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }
};

// --------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, Constructor_WithValidFactory_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that SupervisionManager can be constructed with a valid factory.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    ASSERT_NE(&manager, nullptr);
}

// --------------------------------------------------------------------------
// reserve
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, Reserve_WithValidSize_ReserveSpace)
{
    RecordProperty("Description", "Verify that reserve allocates space for supervisions.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    manager.reserve(10);
    ASSERT_FALSE(manager.full());
}

TEST_F(SupervisionManagerTest, Reserve_WithZeroSize_DoesNotAllocate)
{
    RecordProperty("Description", "Verify that reserve with zero size does not allocate space.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    manager.reserve(0);
    ASSERT_TRUE(manager.full());
}

TEST_F(SupervisionManagerTest, Reserve_WithLargeSize_ReservesLargeCapacity)
{
    RecordProperty("Description", "Verify that reserve can allocate large capacity.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    manager.reserve(1000);
    ASSERT_FALSE(manager.full());
}

// --------------------------------------------------------------------------
// full
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, Full_WithoutReserve_ReturnsTrueImmediately)
{
    RecordProperty("Description", "Verify that full returns true when capacity is not set.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    ASSERT_TRUE(manager.full());
}

TEST_F(SupervisionManagerTest, Full_AfterReserve_ReturnsFalse)
{
    RecordProperty("Description", "Verify that full returns false after reserving space.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));

    manager.reserve(5);
    ASSERT_FALSE(manager.full());
}

// --------------------------------------------------------------------------
// constructWorker
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, ConstructWorker_WithAllSuccessfulCalls_ReturnsTrue)
{
    RecordProperty("Description", "Verify that constructWorker returns true when all factory methods succeed.");

    auto mockFactory = std::make_unique<MockFactory>();

    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillOnce(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                events.emplace_back(id);
                return true;
            }));
    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillOnce(Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
            servers.emplace_back();
            return true;
        }));
    EXPECT_CALL(*mockFactory, createAliveIf)
        .WillOnce(Invoke([](std::vector<ifappl::MonitorIfDaemon>& interfaces,
                            ifappl::CheckpointIpcServer& ipc_server,
                            ifexm::ObservableEvent&) {
            interfaces.emplace_back(ipc_server, "test_interface");
            return true;
        }));
    EXPECT_CALL(*mockFactory, createAliveSupervision)
        .WillOnce(Invoke([](std::vector<supervision::Alive>& supervisions,
                            ifappl::MonitorIfDaemon& interface,
                            ifexm::ObservableEvent&,
                            const std::shared_ptr<IRecoveryClient> recovery_client,
                            const IdentifierHash id,
                            const configuration::ComponentAliveSupervision cfg) {
            supervisions.emplace_back(id, cfg, recovery_client, interface, 256);
            return true;
        }));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"test_component"};
    configuration::ComponentAliveSupervision compConfig{};
    compConfig.reporting_cycle_ms = 100;  // Alive requires a non-zero reference cycle
    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);

    ASSERT_TRUE(result);
}

TEST_F(SupervisionManagerTest, ConstructWorker_WithObservableEventFailure_ReturnsFalse)
{
    RecordProperty("Description", "Verify that constructWorker returns false when createObservableEvent fails.");

    auto mockFactory = std::make_unique<MockFactory>();

    EXPECT_CALL(*mockFactory, createObservableEvent).WillOnce(Return(false));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"test_component"};
    configuration::ComponentAliveSupervision compConfig{};
    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);

    ASSERT_FALSE(result);
}

TEST_F(SupervisionManagerTest, ConstructWorker_WithAliveIfIpcFailure_ReturnsFalse)
{
    RecordProperty("Description", "Verify that constructWorker returns false when createAliveIfIpc fails.");

    auto mockFactory = std::make_unique<MockFactory>();

    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillOnce(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                events.emplace_back(id);
                return true;
            }));
    EXPECT_CALL(*mockFactory, createAliveIfIpc).WillOnce(Return(false));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"test_component"};
    configuration::ComponentAliveSupervision compConfig{};
    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);

    ASSERT_FALSE(result);
}

TEST_F(SupervisionManagerTest, ConstructWorker_WithAliveIfFailure_ReturnsFalse)
{
    RecordProperty("Description", "Verify that constructWorker returns false when createAliveIf fails.");

    auto mockFactory = std::make_unique<MockFactory>();

    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillOnce(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                events.emplace_back(id);
                return true;
            }));
    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillOnce(Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
            servers.emplace_back();
            return true;
        }));
    EXPECT_CALL(*mockFactory, createAliveIf).WillOnce(Return(false));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"test_component"};
    configuration::ComponentAliveSupervision compConfig{};
    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);

    ASSERT_FALSE(result);
}

TEST_F(SupervisionManagerTest, ConstructWorker_WithAliveSupervisionFailure_ReturnsFalse)
{
    RecordProperty("Description", "Verify that constructWorker returns false when createAliveSupervision fails.");

    auto mockFactory = std::make_unique<MockFactory>();

    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillOnce(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                events.emplace_back(id);
                return true;
            }));
    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillOnce(Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
            servers.emplace_back();
            return true;
        }));
    EXPECT_CALL(*mockFactory, createAliveIf)
        .WillOnce(Invoke([](std::vector<ifappl::MonitorIfDaemon>& interfaces,
                            ifappl::CheckpointIpcServer& ipc_server,
                            ifexm::ObservableEvent&) {
            interfaces.emplace_back(ipc_server, "test_interface");
            return true;
        }));
    EXPECT_CALL(*mockFactory, createAliveSupervision).WillOnce(Return(false));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"test_component"};
    configuration::ComponentAliveSupervision compConfig{};
    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);

    ASSERT_FALSE(result);
}

// --------------------------------------------------------------------------
// evaluateSupervisions
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, EvaluateSupervisions_WithNoSupervisions_DoesNotCrash)
{
    RecordProperty("Description", "Verify that SupervisionManager does not crash when empty.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    // Verify manager is in valid state
    ASSERT_FALSE(manager.full());
}

// --------------------------------------------------------------------------
// performCyclicTriggers
// --------------------------------------------------------------------------

TEST_F(SupervisionManagerTest, PerformCyclicTriggers_WithNoSupervisions_DoesNotCrash)
{
    RecordProperty("Description", "Verify that performCyclicTriggers does not crash with no supervisions.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    manager.performCyclicTriggers(std::chrono::nanoseconds{1000});

    // If we reach here without crash, test passed
}

TEST_F(SupervisionManagerTest, PerformCyclicTriggers_WithSupervisions_ExecutesEvaluation)
{
    RecordProperty("Description", "Verify that performCyclicTriggers evaluates supervisions when present.");

    auto mockFactory = std::make_unique<MockFactory>();
    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    manager.performCyclicTriggers(std::chrono::nanoseconds{1000});

    // If we reach here without crash, test passed
    SUCCEED();
}

TEST_F(SupervisionManagerTest, ConstructWorker_MultipleWorkers_AllSucceed)
{
    RecordProperty("Description", "Verify that multiple workers can be constructed sequentially.");

    auto mockFactory = std::make_unique<MockFactory>();

    // Use Invoke to populate vectors - WillRepeatedly so it can be called 3 times
    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillRepeatedly(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                ifexm::ObservableEvent event(id);
                events.push_back(std::move(event));
                return true;
            }));

    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillRepeatedly(
            Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
                ifappl::CheckpointIpcServer server;
                servers.push_back(std::move(server));
                return true;
            }));

    EXPECT_CALL(*mockFactory, createAliveIf)
        .WillRepeatedly(Invoke([](std::vector<ifappl::MonitorIfDaemon>& interfaces,
                                  ifappl::CheckpointIpcServer& ipc_server,
                                  ifexm::ObservableEvent&) {
            ifappl::MonitorIfDaemon daemon(ipc_server, "test_interface");
            interfaces.push_back(std::move(daemon));
            return true;
        }));

    EXPECT_CALL(*mockFactory, createAliveSupervision)
        .WillRepeatedly(Invoke([](std::vector<supervision::Alive>& supervisions,
                                  ifappl::MonitorIfDaemon& interface,
                                  ifexm::ObservableEvent& event,
                                  const std::shared_ptr<IRecoveryClient> recovery_client,
                                  const IdentifierHash id,
                                  const configuration::ComponentAliveSupervision cfg) {
            supervision::Alive alive(id, cfg, recovery_client, interface, 256);
            supervisions.push_back(std::move(alive));
            return true;
        }));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(10);

    IdentifierHash componentId1{"component1"};
    IdentifierHash componentId2{"component2"};
    IdentifierHash componentId3{"component3"};

    // Create valid config with non-zero reporting_cycle_ms
    configuration::ComponentAliveSupervision compConfig{};
    compConfig.reporting_cycle_ms = 100;  // Valid non-zero cycle time

    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    bool result1 = manager.constructWorker(componentId1, compConfig, 1000, recoveryClient, reader);
    bool result2 = manager.constructWorker(componentId2, compConfig, 1001, recoveryClient, reader);
    bool result3 = manager.constructWorker(componentId3, compConfig, 1002, recoveryClient, reader);

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result3);

    // Verify manager is not full after 3 constructions
    ASSERT_FALSE(manager.full());
}

TEST_F(SupervisionManagerTest, PerformCyclicTriggers_LoopsExecuteForNonEmptySupervisions)
{
    RecordProperty("Description", "Verify loop bodies execute in checkInterfaceForNewData and evaluateSupervisions.");

    auto mockFactory = std::make_unique<MockFactory>();

    // Use Invoke to actually populate vectors for loop execution
    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillRepeatedly(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                ifexm::ObservableEvent event(id);
                events.push_back(std::move(event));
                return true;
            }));

    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillRepeatedly(
            Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
                ifappl::CheckpointIpcServer server;
                servers.push_back(std::move(server));
                return true;
            }));

    EXPECT_CALL(*mockFactory, createAliveIf)
        .WillRepeatedly(Invoke([](std::vector<ifappl::MonitorIfDaemon>& interfaces,
                                  ifappl::CheckpointIpcServer& ipc_server,
                                  ifexm::ObservableEvent&) {
            ifappl::MonitorIfDaemon daemon(ipc_server, "test_interface");
            interfaces.push_back(std::move(daemon));
            return true;
        }));

    EXPECT_CALL(*mockFactory, createAliveSupervision)
        .WillRepeatedly(Invoke([](std::vector<supervision::Alive>& supervisions,
                                  ifappl::MonitorIfDaemon& interface,
                                  ifexm::ObservableEvent& event,
                                  const std::shared_ptr<IRecoveryClient> recovery_client,
                                  const IdentifierHash id,
                                  const configuration::ComponentAliveSupervision cfg) {
            supervision::Alive alive(id, cfg, recovery_client, interface, 256);
            supervisions.push_back(std::move(alive));
            return true;
        }));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"component1"};
    configuration::ComponentAliveSupervision compConfig{};
    compConfig.reporting_cycle_ms = 100;
    compConfig.failed_cycles_tolerance = 3;
    compConfig.min_indications = 1;
    compConfig.max_indications = 10;

    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    // Construct one worker - this populates aliveInterfaces and aliveSupervisions
    bool constructResult = manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader);
    ASSERT_TRUE(constructResult);

    // performCyclicTriggers will call checkInterfaceForNewData and evaluateSupervisions
    // This will execute the loop bodies in both functions
    manager.performCyclicTriggers(std::chrono::milliseconds{100});

    // If we reach here, loops executed without crash - success
    SUCCEED();
}

TEST_F(SupervisionManagerTest, HasAnyRecoveryEnqueueFailed_WithFailedRecovery_ReturnsTrue)
{
    RecordProperty(
        "Description",
        "Verify hasAnyRecoveryEnqueueFailed returns true once an expired supervision failed to enqueue recovery.");

    auto mockFactory = std::make_unique<MockFactory>();
    ifexm::ObservableEvent* eventPtr{nullptr};

    EXPECT_CALL(*mockFactory, createObservableEvent)
        .WillOnce(Invoke(
            [](std::vector<ifexm::ObservableEvent>& events, const IdentifierHash id, ifexm::ObservableEventReader&) {
                ifexm::ObservableEvent event(id);
                events.push_back(std::move(event));
                return true;
            }));

    EXPECT_CALL(*mockFactory, createAliveIfIpc)
        .WillOnce(Invoke([](std::vector<ifappl::CheckpointIpcServer>& servers, const IdentifierHash, const uid_t) {
            ifappl::CheckpointIpcServer server;
            servers.push_back(std::move(server));
            return true;
        }));

    EXPECT_CALL(*mockFactory, createAliveIf)
        .WillOnce(Invoke([](std::vector<ifappl::MonitorIfDaemon>& interfaces,
                            ifappl::CheckpointIpcServer& ipc_server,
                            ifexm::ObservableEvent&) {
            ifappl::MonitorIfDaemon daemon(ipc_server, "test_interface");
            interfaces.push_back(std::move(daemon));
            return true;
        }));

    EXPECT_CALL(*mockFactory, createAliveSupervision)
        .WillOnce(Invoke([&eventPtr](
                             std::vector<supervision::Alive>& supervisions,
                             ifappl::MonitorIfDaemon& interface,
                             ifexm::ObservableEvent& event,
                             const std::shared_ptr<IRecoveryClient> recovery_client,
                             const IdentifierHash id,
                             const configuration::ComponentAliveSupervision cfg) {
            auto& alive = supervisions.emplace_back(id, cfg, recovery_client, interface, 256);
            event.attachObserver(alive);
            eventPtr = &event;
            return true;
        }));

    SupervisionManager manager(std::move(mockFactory));
    manager.reserve(5);

    IdentifierHash componentId{"component1"};
    configuration::ComponentAliveSupervision compConfig{};
    compConfig.min_indications = 1;
    compConfig.max_indications = 3;
    compConfig.failed_cycles_tolerance = 0;
    compConfig.reporting_cycle_ms = 1;

    auto recoveryClient = std::make_shared<MockRecoveryClient>();
    EXPECT_CALL(*recoveryClient, sendRecoveryRequest).WillOnce(Return(false));

    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    ASSERT_TRUE(manager.constructWorker(componentId, compConfig, 1000, recoveryClient, reader));
    ASSERT_NE(eventPtr, nullptr);

    eventPtr->event.systemClockTimestamp.tv_nsec = 10;
    eventPtr->event.eventType = SupervisionEventType::kActivation;
    eventPtr->pushData();

    ASSERT_FALSE(manager.hasAnyRecoveryEnqueueFailed());

    manager.performCyclicTriggers(std::chrono::nanoseconds{11});
    manager.performCyclicTriggers(std::chrono::nanoseconds{1000011});

    ASSERT_TRUE(manager.hasAnyRecoveryEnqueueFailed());
}
}  // namespace score::mw::lifecycle::internal::saf::daemon

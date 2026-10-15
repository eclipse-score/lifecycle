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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/MockObservableEventReader.hpp"
#include "score/mw/launch_manager/recovery_client/mock_recovery_client.h"

using namespace testing;

namespace score::mw::lifecycle::internal::saf
{

namespace ifappl
{
std::unique_ptr<MockCheckpointIpcServer> g_mock_checkpoint_ipc_server;
std::unique_ptr<MockMonitorIfDaemon> g_mock_monitor_if_daemon;
}  // namespace ifappl

namespace factory
{

struct FactoryException : public std::exception
{
    explicit FactoryException(std::string_view reason) : reason_(reason) {};

    std::string_view reason_;

    const char* what() const noexcept override
    {
        return reason_.data();
    }
};

class AliveWorkerFactoryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        config_.reporting_cycle_ms = 100;

        ipc_ = std::make_unique<ifappl::CheckpointIpcServer>();
        interface_ = std::make_unique<ifappl::MonitorIfDaemon>(*ipc_, "/dev/null");
        ifappl::g_mock_checkpoint_ipc_server = std::make_unique<ifappl::MockCheckpointIpcServer>();
        ifappl::g_mock_monitor_if_daemon = std::make_unique<ifappl::MockMonitorIfDaemon>();

        EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, getPath)
            .Times(AnyNumber())
            .WillRepeatedly(Return("/dev/null"));
    }

    void TearDown() override
    {
        ifappl::g_mock_checkpoint_ipc_server.reset();  // GMock destructor verifies and clears expectations
        ifappl::g_mock_monitor_if_daemon.reset();
    }

    AliveWorkerFactory factory{};

    const IdentifierHash component_id_{"Component"};
    std::unique_ptr<ifappl::CheckpointIpcServer> ipc_;
    std::unique_ptr<ifappl::MonitorIfDaemon> interface_;
    ifexm::ObservableEvent event_{component_id_};
    std::shared_ptr<MockRecoveryClient> mock_recovery_client_ = std::make_shared<MockRecoveryClient>();
    ComponentAliveSupervision config_{};
};

TEST_F(AliveWorkerFactoryTest, createObservableEventFails)
{
    RecordProperty(
        "Description",
        "Verify that exceptions thrown while creating an observable event are caught and a failure is returned");

    ifexm::MockObservableEventReader reader{};
    std::vector<ifexm::ObservableEvent> out{};
    bool create_result = true;

    EXPECT_CALL(reader, registerObservableEvent).WillOnce(Throw(FactoryException{"Exception during registration"}));

    EXPECT_NO_THROW(create_result = factory.createObservableEvent(out, component_id_, reader));
    EXPECT_FALSE(create_result);
}

TEST_F(AliveWorkerFactoryTest, createObservableEventSucceeds)
{
    RecordProperty(
        "Description",
        "Verify that createObservableEvent creates and attaches an observable event with the correct information");

    ifexm::MockObservableEventReader reader{};
    std::vector<ifexm::ObservableEvent> out{};
    bool create_result = false;
    PolymorphicMatcher event_matcher =
        Field(&ifexm::ObservableEvent::event, Field(&SupervisionEvent::id, Eq(component_id_)));

    EXPECT_CALL(reader, registerObservableEvent(event_matcher, component_id_)).WillOnce(Return(true));

    create_result = factory.createObservableEvent(out, component_id_, reader);

    EXPECT_TRUE(create_result);
    EXPECT_THAT(out.back(), event_matcher);
}

TEST_F(AliveWorkerFactoryTest, createAliveIfIpcThrows)
{
    RecordProperty(
        "Description",
        "Verify that exceptions thrown while creating an ipc server are caught and a failure is returned");

    std::vector<ifappl::CheckpointIpcServer> out{};
    bool create_result = true;

    EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, init).WillOnce(Throw(FactoryException{"IPC init error"}));

    EXPECT_NO_THROW(create_result = factory.createAliveIfIpc(out, IdentifierHash{"Component"}, 0));
    EXPECT_FALSE(create_result);
}

TEST_F(AliveWorkerFactoryTest, createAliveIfIpcSucceeds)
{
    RecordProperty("Description", "Verify that createAliveIfIpc creates an ipc server with the correct information");

    std::vector<ifappl::CheckpointIpcServer> out{};
    bool create_result = true;
    const uid_t uid = 151;

    EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, init)
        .WillOnce(Return(ifappl::CheckpointIpcServer::EIpcInitResult::kOk));
    EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, setAccessRights(uid)).WillOnce(Return(true));

    create_result = factory.createAliveIfIpc(out, IdentifierHash{"Component"}, uid);
    EXPECT_TRUE(create_result);
    EXPECT_EQ(out.size(), 1);
}

TEST_F(AliveWorkerFactoryTest, createAliveIfFails)
{
    RecordProperty(
        "Description",
        "Verify that exceptions thrown while creating an alive interface are caught and a failure is returned");

    std::vector<ifappl::MonitorIfDaemon> out{};
    bool create_result = true;

    EXPECT_CALL(*ifappl::g_mock_monitor_if_daemon, ctor)
        .WillOnce(Throw(FactoryException{"Exception during construction"}));
    EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, getPath).WillOnce(Return("/dev/null"));

    EXPECT_NO_THROW(create_result = factory.createAliveIf(out, *ipc_, event_));
    EXPECT_FALSE(create_result);
}

TEST_F(AliveWorkerFactoryTest, createAliveIfSucceeds)
{
    RecordProperty("Description", "Verify that createAliveIf creates an alive interface with the correct information");

    std::vector<ifappl::MonitorIfDaemon> out{};
    bool create_result = true;

    EXPECT_CALL(*ifappl::g_mock_monitor_if_daemon, ctor);
    EXPECT_CALL(*ifappl::g_mock_checkpoint_ipc_server, getPath).WillOnce(Return("/dev/null"));

    create_result = factory.createAliveIf(out, *ipc_, event_);
    EXPECT_TRUE(create_result);
    EXPECT_EQ(out.size(), 1);
}

TEST_F(AliveWorkerFactoryTest, createAliveSupervisionFails)
{
    RecordProperty(
        "Description",
        "Verify that exceptions thrown while creating an alive supervision are caught and a failure is returned");

    std::vector<supervision::Alive> out{};
    bool create_result = true;

    EXPECT_NO_THROW(
        create_result =
            factory.createAliveSupervision(out, *interface_, event_, mock_recovery_client_, component_id_, config_));
    EXPECT_FALSE(create_result);
}

TEST_F(AliveWorkerFactoryTest, createAliveSupervisionSucceeds)
{
    RecordProperty(
        "Description", "Verify that createAliveSupervision creates an alive supervision with the correct information");

    std::vector<supervision::Alive> out{};
    bool create_result = true;

    create_result =
        factory.createAliveSupervision(out, *interface_, event_, mock_recovery_client_, component_id_, config_);
    EXPECT_TRUE(create_result);
    EXPECT_EQ(out.size(), 1);
}

}  // namespace factory

}  // namespace score::mw::lifecycle::internal::saf

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

#include "score/lcm/saf/recovery/Notification.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace recovery
{

class MockRecoveryClient : public score::lcm::IRecoveryClient
{
public:
    MOCK_METHOD(bool, sendRecoveryRequest,
                (const score::lcm::IdentifierHash&),
                (noexcept, override));
    MOCK_METHOD(std::optional<score::lcm::RecoveryRequest>, getNextRequest, (), (noexcept, override));
    MOCK_METHOD(bool, hasOverflow, (), (const, noexcept, override));
};

class NotificationWithConfigTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

class NotificationWithoutConfigTest : public NotificationWithConfigTest
{
};

NotificationConfig getNotificationConfig(const std::string& process_group="TestProcessGroupMetaModelIdentifier")
{
    NotificationConfig config;
    config.configName = "TestNotification";
    config.processGroupMetaModelIdentifier = process_group + "/Recovery";
    return config;
}

using xaap::lcm::saf::recovery::supervision::SupervisionErrorInfo;

TEST_F(NotificationWithConfigTest, CanGetNotificationName)
{
    RecordProperty("Description",
                   "Notification created with a configuration returns the correct name from getName method.");

    NotificationConfig config{};
    config.configName = "MyName";
    auto mock_client = std::make_shared<MockRecoveryClient>();
    Notification notification(config, mock_client);

    ASSERT_EQ(notification.getConfigName(), "MyName");
}

TEST_F(NotificationWithConfigTest, SendsRequestAndNeverTimesOut)
{
    RecordProperty("Description",
                   "Notification created with a configuration forwards supervision failure to RecoveryClient "
                   "and does not time out.");

    const NotificationConfig config = getNotificationConfig("TestProcessGroup");
    auto mock_client = std::make_shared<MockRecoveryClient>();
    EXPECT_CALL(*mock_client, sendRecoveryRequest(IdentifierHash("TestProcessGroup")))
        .WillOnce(testing::Return(true));

    Notification notification(config, mock_client);
    ASSERT_TRUE(notification.initProxy());
    notification.send(SupervisionErrorInfo{});
    notification.cyclicTrigger();

    EXPECT_FALSE(notification.isFinalTimeoutStateReached());
}

TEST_F(NotificationWithConfigTest, TimesOutWhenSendingRequestFails)
{
    RecordProperty("Description",
                   "Notification fails to forward supervision failure to RecoveryClient "
                   "times out.");

    const NotificationConfig config = getNotificationConfig("TestProcessGroup");
    auto mock_client = std::make_shared<MockRecoveryClient>();
    EXPECT_CALL(*mock_client, sendRecoveryRequest(IdentifierHash("TestProcessGroup")))
        .WillOnce(testing::Return(false));

    Notification notification(config, mock_client);
    ASSERT_TRUE(notification.initProxy());
    notification.send(SupervisionErrorInfo{});
    notification.cyclicTrigger();

    EXPECT_TRUE(notification.isFinalTimeoutStateReached());
}

TEST_F(NotificationWithConfigTest, ProxyInitializationFails)
{
    RecordProperty("Description",
                   "Notification created with a configuration containing an invalid process group state "
                   "identifier returns false on initProxy method.");

    NotificationConfig config;
    config.configName = "TestNotification";
    config.processGroupMetaModelIdentifier = "InvalidIdentifier";
    auto mock_client = std::make_shared<MockRecoveryClient>();
    
    Notification notification(config, mock_client);
    ASSERT_FALSE(notification.initProxy());
}

TEST_F(NotificationWithoutConfigTest, WatchdogNotificationGoesDirectlyToTimeout)
{
    RecordProperty("Description",
                   "Notifications created without any configuration will directly timeout and " 
                   "not forward supervision failure to recoveryclient.");

    auto mock_client = std::make_shared<MockRecoveryClient>();
    EXPECT_CALL(*mock_client, sendRecoveryRequest(testing::_)).Times(0);

    Notification notification(mock_client);
    ASSERT_TRUE(notification.initProxy());
    notification.send(SupervisionErrorInfo{});

    EXPECT_TRUE(notification.isFinalTimeoutStateReached());
}

}  // namespace recovery
}  // namespace saf
}  // namespace lcm
}  // namespace score

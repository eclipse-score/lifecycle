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

#include "score/mw/launch_manager/alive_monitor/details/ipc/IpcServer.hpp"

#include <cerrno>
#include <cstdint>

#include "score/os/utils/acl/access_control_list_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal::saf::ipc
{
namespace
{

struct Payload
{
    std::uint32_t value;
};

using Server = IpcServer<Payload, 4U>;

constexpr uid_t kUid{2121U};

using ::testing::Return;

TEST(IpcServerAclTest, GrantReadWriteAllowsReadAndWriteForUser)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Verify that grantReadWrite grants read and write permission to the given user.");

    score::os::AccessControlListMock acl;
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kRead))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kWrite))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_TRUE(Server::grantReadWrite(acl, kUid));
}

TEST(IpcServerAclTest, GrantReadWriteFailsAndSkipsWriteIfReadFails)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Verify that grantReadWrite returns false and does not grant write if read fails.");

    score::os::AccessControlListMock acl;
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kRead))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kWrite)).Times(0);

    EXPECT_FALSE(Server::grantReadWrite(acl, kUid));
}

TEST(IpcServerAclTest, GrantReadWriteFailsIfWriteFails)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Verify that grantReadWrite returns false if granting write fails.");

    score::os::AccessControlListMock acl;
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kRead))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));
    EXPECT_CALL(acl, AllowUser(kUid, score::os::Acl::Permission::kWrite))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    EXPECT_FALSE(Server::grantReadWrite(acl, kUid));
}

TEST(IpcServerTest, SetAccessRightsFailsIfSharedMemoryDoesNotExist)
{
    RecordProperty(
        "Description",
        "Verify that setAccessRights returns false if the shared "
        "memory has not been created.");

    Server server;
    EXPECT_FALSE(server.setAccessRights(2121U));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::saf::ipc

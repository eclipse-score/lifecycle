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

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "score/os/utils/acl/access_control_list.h"

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

/// @brief Uses the real shared memory socket and inspects the resulting ACL
///        of the shm file via the baselibs AccessControlList.
class IpcServerAclFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
        m_name = "/ipc_server_ut_" + std::to_string(::getpid());
        ASSERT_EQ(m_server.init(m_name), Server::EIpcInitResult::kOk);
    }

    void TearDown() override
    {
        ::shm_unlink(m_name.c_str());
    }

    /// @brief Permissions of the ACL as seen through AccessControlList.
    struct AclView
    {
        std::vector<uid_t> users_with_read;
        std::vector<uid_t> users_with_write;
        bool mask_allows_read_write{false};
    };

    /// @brief Read the acl permissions.
    AclView readAcl()
    {
        AclView ouput{};
        const int fd = ::shm_open(m_name.c_str(), O_RDWR, 0);
        EXPECT_GE(fd, 0);

        const score::os::AccessControlList acl{static_cast<score::os::Acl::FileDescriptor>(fd)};
        const auto read = acl.FindUserIdsWithPermission(score::os::Acl::Permission::kRead);
        const auto write = acl.FindUserIdsWithPermission(score::os::Acl::Permission::kWrite);
        const auto mask =
            acl.VerifyMaskPermissions({score::os::Acl::Permission::kRead, score::os::Acl::Permission::kWrite});

        EXPECT_TRUE(read.has_value());
        EXPECT_TRUE(write.has_value());
        EXPECT_TRUE(mask.has_value());

        ouput = AclView{read.value(), write.value(), mask.value()};
        ::close(fd);
        return ouput;
    }

    /// @brief Checks if the given uid exists in the list.
    static bool contains(const std::vector<uid_t>& uids, uid_t uid)
    {
        return std::find(uids.begin(), uids.end(), uid) != uids.end();
    }

    /// @brief Name of the shm.
    std::string m_name;

    /// @brief The IPC server.
    Server m_server;
};

TEST_F(IpcServerAclFixture, SetAccessRightsGrantsReadWriteToGivenUser)
{
    RecordProperty(
        "Description",
        "Verify that setAccessRights grants read and write access to the given "
        "user on the shared memory ACL and that the ACL mask permits read and "
        "write.");

    constexpr uid_t kUid{2121U};
    ASSERT_TRUE(m_server.setAccessRights(kUid));

    const AclView acl = readAcl();
    EXPECT_TRUE(contains(acl.users_with_read, kUid));
    EXPECT_TRUE(contains(acl.users_with_write, kUid));
    EXPECT_TRUE(acl.mask_allows_read_write);
}

TEST_F(IpcServerAclFixture, SetAccessRightsOnSecondCallGrantsAccessToLatestUser)
{
    RecordProperty(
        "Description",
        "Verify that calling setAccessRights a second time for a different user"
        "grants that user read and write access.");

    ASSERT_TRUE(m_server.setAccessRights(2121U));
    ASSERT_TRUE(m_server.setAccessRights(2122U));

    const AclView acl = readAcl();
    EXPECT_TRUE(contains(acl.users_with_read, 2122U));
    EXPECT_TRUE(contains(acl.users_with_write, 2122U));
    EXPECT_TRUE(acl.mask_allows_read_write);
}

TEST_F(IpcServerAclFixture, SetAccessRightsIsIdempotent)
{
    RecordProperty(
        "Description",
        "Verify that calling setAccessRights repeatedly for the same user "
        "succeeds and the user keeps read and write access.");

    ASSERT_TRUE(m_server.setAccessRights(2121U));
    ASSERT_TRUE(m_server.setAccessRights(2121U));

    const AclView acl = readAcl();
    EXPECT_TRUE(contains(acl.users_with_read, 2121U));
    EXPECT_TRUE(contains(acl.users_with_write, 2121U));
}

TEST(IpcServerTest, SetAccessRightsFailsIfSharedMemoryDoesNotExist)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing ");
    RecordProperty(
        "Description",
        "Verify that setAccessRights returns false if the shared "
        "memory has not been created.");

    Server server;
    EXPECT_FALSE(server.setAccessRights(2121U));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::saf::ipc

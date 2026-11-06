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

#include "score/os/acl.h"
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
        RecordProperty("DerivationTechnique", "explorative-testing");
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

    /// @brief Whether the owning group and others have any permission in the ACL.
    struct GroupOtherAccess
    {
        bool group{true};
        bool other{true};
    };

    /// @brief Read the owning group and other ACL entries of the given file
    ///        descriptor.
    static GroupOtherAccess getGroupOther(const int fd)
    {
        GroupOtherAccess output{};
        auto& os = score::os::Acl::instance();
        auto raw_acl = os.acl_get_fd(fd);
        EXPECT_TRUE(raw_acl.has_value());
        if (!raw_acl.has_value())
        {
            return output;
        }

        for (auto entry = os.acl_get_entry(raw_acl.value(), score::os::Acl::kAclFirstEntry);
             entry.has_value() && entry.value().has_value();
             entry = os.acl_get_entry(raw_acl.value(), score::os::Acl::kAclNextEntry))
        {
            const auto tag = os.acl_get_tag_type(entry.value().value());
            const bool is_group = tag.has_value() && tag.value() == score::os::Acl::Tag::kOwningGroup;
            const bool is_other = tag.has_value() && tag.value() == score::os::Acl::Tag::kOther;
            if (!is_group && !is_other)
            {
                continue;
            }

            score::os::Acl::Permissions permset{};
            os.acl_get_permset(entry.value().value(), &permset);
            bool any{false};
            for (const auto perm :
                 {score::os::Acl::Permission::kRead,
                  score::os::Acl::Permission::kWrite,
                  score::os::Acl::Permission::kExecute})
            {
                // check if there are any permissions set
                any = any || os.acl_get_perm(permset, perm).value();
            }

            if (is_group)
            {
                output.group = any;
            }
            else
            {
                output.other = any;
            }
        }
        os.acl_free(raw_acl.value());
        return output;
    }

    /// @brief Read the acl permissions.
    AclView readAcl()
    {
        AclView ouput{};
        const int fd = ::shm_open(m_server.getPath().data(), O_RDWR, 0);
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

TEST_F(IpcServerAclFixture, SetAccessRightsKeepsGroupAndOtherWithoutAccess)
{
    RecordProperty(
        "Description",
        "Verify that for a shared memory created with mode 0600, setAccessRights"
        "does not grant any permission to the owning group or to others.");

    ASSERT_TRUE(m_server.setAccessRights(54321U));

    // open the shared memory owned by the server, using the path reported by the server itself
    const int fd = ::shm_open(m_server.getPath().data(), O_RDWR, 0);
    ASSERT_GE(fd, 0);
    const GroupOtherAccess access = getGroupOther(fd);
    ::close(fd);

    EXPECT_FALSE(access.group);
    EXPECT_FALSE(access.other);
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

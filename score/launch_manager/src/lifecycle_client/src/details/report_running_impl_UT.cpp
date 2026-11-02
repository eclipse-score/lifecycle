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

#include "score/mw/lifecycle/execution_error.h"
#include "score/mw/lifecycle/lifecycle_client/details/report_running_impl.hpp"
#include "score/os/mocklib/semaphoremock.h"
#include "tests/utils/accessors/accessor_report_running_impl.hpp"
#include "tests/utils/mocks/mock_syscalls.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;

namespace
{
using namespace score::mw::lifecycle::internal::osal;

// Storage for a fake IpcCommsSync object
alignas(IpcCommsSync) static char g_fake_ipc_storage[sizeof(IpcCommsSync)];

// Helper to create a fake IPC structure with specified properties
void* CreateFakeIpcComms(
    std::uint32_t magic = IpcCommsSync::kMagic,
    std::uint32_t version = IpcCommsSync::kLayoutVersion)
{
    std::memset(g_fake_ipc_storage, 0, sizeof(IpcCommsSync));
    IpcCommsSync* fake_comms = reinterpret_cast<IpcCommsSync*>(g_fake_ipc_storage);
    fake_comms->magic_ = magic;
    fake_comms->version_ = version;

    return g_fake_ipc_storage;
}

const score::cpp::expected_blank<score::os::Error> kSemSuccess{};

score::cpp::expected_blank<score::os::Error> SemError(std::int32_t err)
{
    return score::cpp::make_unexpected(score::os::Error::createFromErrno(err));
}

}  // anonymous namespace

namespace score::mw::lifecycle
{

class ReportRunningImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        g_syscall_mock = std::make_unique<SyscallMock>();

        // We need to reset the static reported state for each test run,
        // so each test starts with a consistent state
        ReportRunningImplTestAccessor::SetReportedForTesting(false);

        // Set up default successful behaviors for syscalls
        SetupDefaultSuccessExpectations();
    }

    void TearDown() override
    {
        g_syscall_mock.reset();
    }

    void SetupDefaultSuccessExpectations()
    {
        using ::testing::_;
        using ::testing::Invoke;

        // By default, fstat succeeds and returns valid size
        EXPECT_CALL(*g_syscall_mock, fstat(_, _)).WillRepeatedly(DoAll(SetArgPointee<1>(CreateValidStat()), Return(0)));

        // By default, close succeeds
        EXPECT_CALL(*g_syscall_mock, close(_)).WillRepeatedly(Return(0));

        // By default, mmap succeeds and returns valid IPC structure
        EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
            .WillRepeatedly(Invoke([](void*, size_t length, int, int, int, off_t) -> void* {
                if (length == sizeof(IpcCommsSync))
                {
                    return CreateFakeIpcComms();
                }
                return MAP_FAILED;
            }));

        // By default, sem_post() (backing Semaphore::post()) succeeds
        EXPECT_CALL(*semaphore_mock_, sem_post(_)).WillRepeatedly(Return(kSemSuccess));
    }

    static struct stat CreateValidStat()
    {
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        st.st_size = sizeof(IpcCommsSync);
        return st;
    }

    score::os::MockGuard<score::os::SemaphoreMock, score::os::Semaphore> semaphore_mock_;
};

/****************************************
 * ReportRunningImpl constructor tests
 ****************************************/

TEST_F(ReportRunningImplTest, GivenNoParameters_WhenReportRunningImplConstructed)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given no constructor parameters. "
        "When ReportRunningImpl is constructed. "
        "Then no exceptions are thrown. ");

    // Given, When, Then
    EXPECT_NO_THROW({ ReportRunningImpl impl; });
}

TEST_F(ReportRunningImplTest, GivenMultipleReportRunningImplInstances_WhenReportRunningImplConstructed)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given multiple instances of ReportRunningImpl. "
        "When constructed. "
        "Then no exceptions are thrown. ");

    // Given, When, Then
    EXPECT_NO_THROW({
        ReportRunningImpl impl1;
        ReportRunningImpl impl2;
        ReportRunningImpl impl3;
    });
}

/*********************************************
 * ReportRunningImpl ReportRunningState tests
 *********************************************/

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance with valid IPC comms. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() succeeds. ");

    // Given
    ReportRunningImpl impl;

    // When
    auto result = impl.ReportRunningState();

    // Then
    ASSERT_TRUE(result.has_value());
}

TEST_F(ReportRunningImplTest, GivenConstInstance_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a const ReportRunningImpl instance. "
        "When ReportRunningState is called. "
        "Then no exceptions are thrown. ");

    // Given
    const ReportRunningImpl impl;

    // When, Then
    EXPECT_NO_THROW({ auto result = impl.ReportRunningState(); });
}

TEST_F(ReportRunningImplTest, GivenMultipleInstances_WhenReportRunningStateCalled_ThenStaticStateIsShared)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given multiple ReportRunningImpl instances. "
        "When ReportRunningState() is called from both instances. "
        "Then first succeeds, second returns kInvalidTransition (static state is shared). ");

    // Given
    ReportRunningImpl impl1;
    ReportRunningImpl impl2;

    // When
    auto result1 = impl1.ReportRunningState();
    auto result2 = impl2.ReportRunningState();

    // Then
    EXPECT_TRUE(result1.has_value());   // First call succeeds
    EXPECT_FALSE(result2.has_value());  // Second call fails (already reported)
    EXPECT_EQ(result2.error(), ExecErrc::kInvalidTransition);
}

/****************************************
 * IPC and syscall error path tests
 ****************************************/

TEST_F(ReportRunningImplTest, GivenInvalidFileDescriptor_WhenFstatFails_ThenReturnsCommunicationError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect fstat() to fail with errno EBADF. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() should return kCommunicationError. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, fstat(_, _)).WillOnce(DoAll(SetErrnoAndReturn(EBADF, -1)));

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(
    ReportRunningImplTest,
    GivenReportRunningImplInstance_ExpectFstatToReturnTooSmallSize_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect fstat() to return a size less than sizeof(IpcCommsSync). "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    struct stat small_stat;
    std::memset(&small_stat, 0, sizeof(small_stat));
    small_stat.st_size = sizeof(IpcCommsSync) - 1;

    EXPECT_CALL(*g_syscall_mock, fstat(_, _)).WillOnce(DoAll(SetArgPointee<1>(small_stat), Return(0)));

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_ExceptGetCommsObjectFailure_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect getCommsObject() to fail. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _)).WillOnce(Return(MAP_FAILED));

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenWrongMagic_WhenReportRunningStateCalled_ThenReturnsCommunicationError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect the shared memory not to contain the Launch Manager magic number. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error and does not post the semaphore. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
        .WillOnce([](void*, size_t length, int, int, int, off_t) -> void* {
            if (length == sizeof(IpcCommsSync))
            {
                return CreateFakeIpcComms(0U, IpcCommsSync::kLayoutVersion);
            }
            return MAP_FAILED;
        });
    EXPECT_CALL(*semaphore_mock_, sem_post(_)).Times(0);

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenWrongLayoutVersion_WhenReportRunningStateCalled_ThenReturnsCommunicationError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect the shared memory to contain a different layout version. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error and does not post the semaphore. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
        .WillOnce([](void*, size_t length, int, int, int, off_t) -> void* {
            if (length == sizeof(IpcCommsSync))
            {
                return CreateFakeIpcComms(IpcCommsSync::kMagic, IpcCommsSync::kLayoutVersion + 1U);
            }
            return MAP_FAILED;
        });
    EXPECT_CALL(*semaphore_mock_, sem_post(_)).Times(0);

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_ExpectCloseEBadF_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance."
        "Expect close() to fail with errno EBADF."
        "When ReportRunningState() is called."
        "Then ReportRunningState() returns a kCommunicationError error.");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, close(_)).WillOnce(DoAll(SetErrnoAndReturn(EBADF, -1)));

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_ExpectPostFails_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect send_sync_->post() to fail. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error.");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*semaphore_mock_, sem_post(_)).WillOnce(Return(SemError(EINVAL)));

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_WhenReportRunningStateCalled_ThenPostsExactlyOnce)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance with valid IPC comms. "
        "When ReportRunningState() is called. "
        "Then the semaphore is posted exactly once and the client does not wait for any reply. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*semaphore_mock_, sem_post(_)).Times(1).WillOnce(Return(kSemSuccess));
    EXPECT_CALL(*semaphore_mock_, sem_timedwait_monotonic(_, _)).Times(0);
    EXPECT_CALL(*semaphore_mock_, sem_wait(_)).Times(0);

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_TRUE(result.has_value());
}

}  // namespace score::mw::lifecycle

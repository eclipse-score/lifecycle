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
#include "tests/utils/accessors/accessor_report_running_impl.hpp"
#include "tests/utils/mocks/mock_semaphore.hpp"
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
void* CreateFakeIpcComms(CommsType comms_type, pid_t pid)
{
    std::memset(g_fake_ipc_storage, 0, sizeof(IpcCommsSync));

    // Set pid_ and comms_type_
    // Structure layout: reply_sync_, send_sync_, pid_, comms_type_
    char* base = g_fake_ipc_storage;
    const size_t semaphore_size = sizeof(Semaphore);

    ProcessID* pid_ptr = reinterpret_cast<ProcessID*>(base + 2 * semaphore_size);
    CommsType* type_ptr = reinterpret_cast<CommsType*>(base + 2 * semaphore_size + sizeof(ProcessID));

    *pid_ptr = pid;
    *type_ptr = comms_type;

    return g_fake_ipc_storage;
}

}  // anonymous namespace

// Semaphore::post() / Semaphore::timedWait() are overridden in tests/utils/mocks/mock_semaphore.cpp, controlled via
// the score::mw::lifecycle::internal::osal::mock state below.
using namespace score::mw::lifecycle::internal::osal::mock;

namespace score::mw::lifecycle
{

// Access to the static reported flag for testing
// This is defined in the real implementation
namespace
{
// Helper to reset static state between tests
// We'll use fork or process isolation in Phase 3-4
// For now, we document that tests may have interdependencies
}

class ReportRunningImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        g_syscall_mock = std::make_unique<SyscallMock>();
        ReportRunningImplTestAccessor::SetReportedForTesting(false);
        ResetSemaphoreMockState();

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

        // By default, getpid returns current PID
        EXPECT_CALL(*g_syscall_mock, getpid()).WillRepeatedly(Return(::getpid()));

        // By default, mmap succeeds and returns valid IPC structure
        EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
            .WillRepeatedly(Invoke([](void*, size_t length, int, int, int, off_t) -> void* {
                if (length == sizeof(IpcCommsSync))
                {
                    return CreateFakeIpcComms(CommsType::kReporting, ::getpid());
                }
                return MAP_FAILED;
            }));
    }

    static struct stat CreateValidStat()
    {
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        st.st_size = sizeof(IpcCommsSync);
        return st;
    }
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

TEST_F(ReportRunningImplTest, GivenReportRunningImplInstance_ExpectCommsTypeIsKNoComms_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect a comms_type mismatch. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns kCommunicationError. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
        .WillOnce([](void*, size_t length, int, int, int, off_t) -> void* {
            if (length == sizeof(IpcCommsSync))
            {
                return CreateFakeIpcComms(CommsType::kNoComms, ::getpid());
            }
            return MAP_FAILED;
        });

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenWrongProcessId_WhenPidDoesNotMatch_ThenReturnsCommunicationError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instace. "
        "Expect a pid mismatch. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, _, _))
        .WillOnce([](void*, size_t length, int, int, int, off_t) -> void* {
            if (length == sizeof(IpcCommsSync))
            {
                return CreateFakeIpcComms(CommsType::kReporting, 99999);
            }
            return MAP_FAILED;
        });

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
    // Note: Branch 88:9 (comms_type != kReporting) cannot be covered because
    // if we reach line 88, we must have passed the check at line 81 which
    // guarantees comms_type == kReporting. This is defensive/redundant code.

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
    g_first_post_should_fail = true;

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(
    ReportRunningImplTest,
    GivenReportRunningImplInstance_ExpectReplySyncTimedWaitFailure_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect reply_sync_.timedWait() to fail. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError error. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    g_timedwait_should_fail = true;

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenFinalPostFails_WhenPostReturnsKFail_ThenReturnsCommunicationError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "Expect final send_sync_.post() to fail. "
        "When ReportRunningState() is called. "
        "Then ReportRunningState() returns a kCommunicationError. ");

    // Given
    ReportRunningImpl impl;

    // Expect
    g_second_post_should_fail = true;

    // When
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ReportRunningImplTest, GivenReportRunningImpl_WhenGetReportedForTestingIsInitiallyCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a ReportRunningImpl instance. "
        "When ReportRunningImpl::GetReportedForTesting() is initially called. "
        "Then ReportRunningImpl::GetReportedForTesting() returns false.");

    // Given, When, Then
    EXPECT_FALSE(ReportRunningImplTestAccessor::GetReportedForTesting());
}

TEST_F(ReportRunningImplTest, GivenReportedFlagTrue_WhenReportRunningStateCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given the reported flag is set to true. "
        "When ReportRunningState is called. "
        "Then ReportRunningState returns kInvalidTransition. ");

    // Given
    ReportRunningImplTestAccessor::SetReportedForTesting(true);

    // When
    ReportRunningImpl impl;
    auto result = impl.ReportRunningState();

    // Then
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kInvalidTransition);
}

}  // namespace score::mw::lifecycle

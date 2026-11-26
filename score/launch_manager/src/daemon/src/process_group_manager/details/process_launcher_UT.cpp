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

#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cerrno>
#include <cstdarg>
#include <thread>

#include "score/mw/launch_manager/osal/mock_start_process.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/os/mocklib/semaphoremock.h"
#include "tests/utils/mocks/mock_syscalls.hpp"

using namespace testing;

using namespace score::mw::lifecycle::internal::osal;
using namespace score::mw::lifecycle::internal;

namespace
{
const score::cpp::expected_blank<score::os::Error> kSemSuccess{};

score::cpp::expected_blank<score::os::Error> SemError(std::int32_t err)
{
    return score::cpp::make_unexpected(score::os::Error::createFromErrno(err));
}
}  // namespace

class ProcessLauncherTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        g_syscall_mock = std::make_unique<SyscallMock>();
        process_launcher_ = std::make_unique<ProcessLauncher>();

        // Used by logging framework
        ON_CALL(*g_syscall_mock, access).WillByDefault(Invoke(__real_access));
        ON_CALL(*g_syscall_mock, fcntl).WillByDefault(Invoke(__real_fcntl));
        EXPECT_CALL(*g_syscall_mock, access).Times(AnyNumber());
        EXPECT_CALL(*g_syscall_mock, fcntl).Times(AnyNumber());

        ON_CALL(*semaphore_mock_, sem_init).WillByDefault(Return(kSemSuccess));
        ON_CALL(*semaphore_mock_, sem_post).WillByDefault(Return(kSemSuccess));
        ON_CALL(*semaphore_mock_, sem_wait).WillByDefault(Return(kSemSuccess));
        ON_CALL(*semaphore_mock_, sem_timedwait_monotonic).WillByDefault(Return(kSemSuccess));
    }

    void TearDown() override
    {
        process_launcher_.reset();
        g_syscall_mock.reset();
    }

    std::shared_ptr<IpcCommsSync> GetInitialisedIpc()
    {
        std::memset(test_buffer_.data(), 0, test_buffer_.size());
        auto* sync = reinterpret_cast<IpcCommsSync*>(test_buffer_.data());

        EXPECT_EQ(sync->send_sync_.init(0, false), OsalReturnType::kSuccess);
        // Real msync calls would treat this fake shared memory as invalid because it isn't page aligned.
        ON_CALL(*g_syscall_mock, msync(sync, _, _)).WillByDefault(Return(0));
        EXPECT_CALL(*g_syscall_mock, msync).Times(AnyNumber());

        std::shared_ptr<IpcCommsSync> shared{sync, [](IpcCommsSync*) {
                                             }};

        return shared;
    }

    score::os::MockGuard<NiceMock<score::os::SemaphoreMock>, score::os::Semaphore> semaphore_mock_;
    std::unique_ptr<ProcessLauncher> process_launcher_;

    alignas(IpcCommsSync) std::array<std::byte, sizeof(IpcCommsSync)> test_buffer_;
};

TEST_F(ProcessLauncherTest, waitForTerminationSuccess)
{
    RecordProperty("Description", "Test that waitForTermination calls `wait` and sets the correct values");

    const pid_t pid = 7;
    const uint32_t status = 9;

    EXPECT_CALL(*g_syscall_mock, wait).WillOnce(DoAll(SetArgPointee<0>(status), Return(pid)));

    ProcessID out_pid;
    int32_t out_status;
    const auto res = process_launcher_->waitForTermination(out_pid, out_status);

    EXPECT_EQ(out_pid, pid);
    EXPECT_EQ(out_status, status);
    EXPECT_EQ(res, OsalReturnType::kSuccess);
}

TEST_F(ProcessLauncherTest, waitForTerminationFails)
{
    RecordProperty("Description", "Test that waitForTermination reacts correctly to a failed `wait` syscall");

    EXPECT_CALL(*g_syscall_mock, wait).WillOnce(SetErrnoAndReturn(WNOHANG, -1));

    ProcessID out_pid;
    int32_t out_status;
    const auto res = process_launcher_->waitForTermination(out_pid, out_status);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

class TerminationTest : public ProcessLauncherTest
{
};

TEST_F(TerminationTest, requestTerminationSuccess)
{
    RecordProperty(
        "Description",
        "Test that requestTermination invokes `kill` with the provided pid and SIGTERM and returns the correct osal "
        "result");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill(pid, SIGTERM)).WillOnce(Return(0));

    const OsalReturnType res = process_launcher_->requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kSuccess);
}

TEST_F(TerminationTest, requestTerminationFailure)
{
    RecordProperty(
        "Description", "Test that requestTermination returns the correct osal result when the `kill` syscall fails");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill).WillOnce(SetErrnoAndReturn(ESRCH, -1));

    const OsalReturnType res = process_launcher_->requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, requestTerminationInvalid)
{
    RecordProperty("Description", "Test that requestTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher_->requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, forceTerminationSuccess)
{
    RecordProperty(
        "Description",
        "Test that forceTermination invokes `kill` with the provided pid and SIGKILL and returns the correct osal "
        "result");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill(pid, SIGKILL)).WillOnce(Return(0));

    const OsalReturnType res = process_launcher_->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kSuccess);
}

TEST_F(TerminationTest, forceTerminationSearchFailure)
{
    RecordProperty(
        "Description",
        "Test that forceTermination returns the correct osal result when the `kill` syscall fails due to a missing "
        "process");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill).WillOnce(SetErrnoAndReturn(ESRCH, -1));

    const OsalReturnType res = process_launcher_->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, forceTerminationPermFailure)
{
    RecordProperty(
        "Description",
        "Test that forceTermination returns the correct osal result when the `kill` syscall fails due to a permission "
        "error");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill).WillOnce(SetErrnoAndReturn(EPERM, -1));

    const OsalReturnType res = process_launcher_->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, forceTerminationInvalid)
{
    RecordProperty("Description", "Test that forceTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher_->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

class StartProcessTest : public ProcessLauncherTest
{
  protected:
    void SetUp() override
    {
        ProcessLauncherTest::SetUp();

        g_start_process_mock = std::make_unique<StrictMock<StartProcessMock>>();

        config_.name = "TestComponent";
        config_.component_properties.application_profile.application_type = configuration::ApplicationType::Native;
        config_.deployment_config.process_command = configuration::ProcessCommand{"/bin/TestProcess", {}};
    }

    void TearDown() override
    {
        sync_.reset();
        g_start_process_mock.reset();

        ProcessLauncherTest::TearDown();
    }

    IpcCommsSync* StubShmObject()
    {
        void* data = static_cast<void*>(test_buffer_.data());
        EXPECT_CALL(*g_syscall_mock, shm_open_anon).WillOnce(Return(kShmFd));
        EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, kShmFd, _)).WillOnce(Return(data));
        EXPECT_CALL(*g_syscall_mock, ftruncate(kShmFd, _)).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, munmap(data, sizeof(IpcCommsSync))).WillOnce(Return(0));

        return static_cast<IpcCommsSync*>(data);
    }

    static constexpr int kShmFd = 123;
    const pid_t started_pid_ = 23;

    configuration::ComponentConfig config_ = {};
    ProcessID pid_;
    IpcCommsP sync_;
};

TEST_F(StartProcessTest, startProcessNoFile)
{
    RecordProperty("Description", "Test that startProcess returns a failure if the provided path does not exist");

    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(*g_start_process_mock, startProcess).Times(0);

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(StartProcessTest, startProcessOsalFailed)
{
    RecordProperty("Description", "Test that startProcess returns a failure if no process could be created");

    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*g_start_process_mock, startProcess).WillOnce(Return(OsalReturnType::kFail));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(StartProcessTest, startProcessNativeHasNoComms)
{
    RecordProperty(
        "Description",
        "Test that a native process is started without a shared memory block and that its pid is returned");

    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, shm_open_anon).Times(0);
    EXPECT_CALL(*g_start_process_mock, startProcess(_, Ref(config_), -1, IsNull()))
        .WillOnce(DoAll(SetArgReferee<0>(started_pid_), Return(OsalReturnType::kSuccess)));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, started_pid_);
    EXPECT_EQ(sync_, nullptr);
}

TEST_F(StartProcessTest, startProcessReportingPassesComms)
{
    RecordProperty(
        "Description",
        "Test that a reporting process is started with the shared memory block and fd, and that the fd is closed "
        "afterwards");

    config_.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;

    IpcCommsSync* block = StubShmObject();
    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*semaphore_mock_, sem_init).Times(1).WillRepeatedly(Return(kSemSuccess));
    EXPECT_CALL(*g_start_process_mock, startProcess(_, Ref(config_), kShmFd, Property(&IpcCommsP::get, block)))
        .WillOnce(DoAll(SetArgReferee<0>(started_pid_), Return(OsalReturnType::kSuccess)));
    EXPECT_CALL(*g_syscall_mock, close(kShmFd)).WillOnce(Return(0));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, started_pid_);
    EXPECT_EQ(sync_.get(), block);
    EXPECT_EQ(block->magic_, IpcCommsSync::kMagic);
    EXPECT_EQ(block->version_, IpcCommsSync::kLayoutVersion);
}

class SetupCommsTest : public StartProcessTest
{
    void SetUp() override
    {
        StartProcessTest::SetUp();

        config_.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;

        EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
        EXPECT_CALL(*g_start_process_mock, startProcess).Times(0);
    }
};

TEST_F(SetupCommsTest, shmOpenFails)
{
    RecordProperty("Description", "Verify that if opening shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open_anon).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, ftruncateFails)
{
    RecordProperty("Description", "Verify that if truncating shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open_anon).WillOnce(Return(123));
    EXPECT_CALL(*g_syscall_mock, ftruncate).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, getCommsFails)
{
    RecordProperty("Description", "Verify that if truncating shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open_anon).WillOnce(Return(123));
    EXPECT_CALL(*g_syscall_mock, ftruncate).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, mmap).WillOnce(Return(MAP_FAILED));

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, initSemaphoresFails)
{
    RecordProperty(
        "Description",
        "Verify that if setting up the semaphores fails, startProcess fails, the shared memory is not marked as a "
        "valid comms object and no semaphore is destroyed");

    IpcCommsSync* block = StubShmObject();
    block->magic_ = 0U;
    EXPECT_CALL(*semaphore_mock_, sem_init).WillRepeatedly(Return(SemError(EINVAL)));
    EXPECT_CALL(*g_syscall_mock, sem_destroy).Times(0);

    EXPECT_EQ(process_launcher_->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
    EXPECT_EQ(block->magic_, 0U);
}

TEST_F(ProcessLauncherTest, releasingCommsObjectDestroysSemaphore)
{
    RecordProperty(
        "Description",
        "Verify that the comms object created for Launch Manager destroys its semaphore when the last reference is "
        "dropped");

    const int fd = 123;
    void* data = static_cast<void*>(test_buffer_.data());
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, fd, _)).WillOnce(Return(data));
    EXPECT_CALL(*semaphore_mock_, sem_init).WillOnce(Return(kSemSuccess));

    // Fill with garbage to verify that reported_running_ is initialized explicitly
    std::memset(test_buffer_.data(), 0xFF, test_buffer_.size());

    IpcCommsP block = IpcCommsSync::createCommsObject(fd);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->magic_, IpcCommsSync::kMagic);
    EXPECT_EQ(block->version_, IpcCommsSync::kLayoutVersion);
    EXPECT_FALSE(block->hasReportedRunning());

    EXPECT_CALL(*g_syscall_mock, msync(data, _, _)).WillOnce(Return(0));
    Expectation destroyed = EXPECT_CALL(*g_syscall_mock, sem_destroy).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, munmap(data, sizeof(IpcCommsSync))).After(destroyed).WillOnce(Return(0));

    block.reset();
}

TEST_F(ProcessLauncherTest, releasingCommsObjectSkipsDestroyOnInvalidMemory)
{
    RecordProperty(
        "Description",
        "Verify that the semaphore is not destroyed if the shared memory appears invalid, but memory is still "
        "unmapped");

    const int fd = 123;
    void* data = static_cast<void*>(test_buffer_.data());
    EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, fd, _)).WillOnce(Return(data));
    EXPECT_CALL(*semaphore_mock_, sem_init).WillOnce(Return(kSemSuccess));

    IpcCommsP block = IpcCommsSync::createCommsObject(fd);
    ASSERT_NE(block, nullptr);

    EXPECT_CALL(*g_syscall_mock, msync(data, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, sem_destroy).Times(0);
    EXPECT_CALL(*g_syscall_mock, munmap(data, sizeof(IpcCommsSync))).WillOnce(Return(0));

    block.reset();
}

class ClientMethodsTest : public ProcessLauncherTest
{
};

using namespace std::chrono_literals;

TEST_F(ProcessLauncherTest, kRunningNoSync)
{
    RecordProperty("Description", "Verify that waitForkRunning correctly handles a null pointer");

    std::shared_ptr<IpcCommsSync> sync;

    EXPECT_EQ(process_launcher_->waitForkRunning(sync, 1ms), OsalReturnType::kFail);
}

TEST_F(ProcessLauncherTest, kRunningSuccess)
{
    RecordProperty(
        "Description", "Verify that waitForkRunning waits for exactly one notification and does not post a reply");

    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();
    sync->reportRunning();

    EXPECT_CALL(*semaphore_mock_, sem_timedwait_monotonic).Times(1).WillOnce(Return(kSemSuccess));
    EXPECT_CALL(*semaphore_mock_, sem_post).Times(0);

    EXPECT_EQ(process_launcher_->waitForkRunning(sync, 1ms), OsalReturnType::kSuccess);
}

TEST_F(ProcessLauncherTest, kRunningWokenUpWithoutReport)
{
    RecordProperty(
        "Description",
        "Verify that waitForkRunning fails if the semaphore was posted without kRunning being reported, as happens "
        "when Launch Manager wakes up the waiter because the process terminated");

    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();

    EXPECT_CALL(*semaphore_mock_, sem_timedwait_monotonic).Times(1).WillOnce(Return(kSemSuccess));

    EXPECT_FALSE(sync->hasReportedRunning());
    EXPECT_EQ(process_launcher_->waitForkRunning(sync, 1ms), OsalReturnType::kFail);
}

TEST_F(ProcessLauncherTest, kRunningTimeout)
{
    RecordProperty(
        "Description",
        "Verify that waitForkRunning returns a timeout failure if no notification is received within the timeout");

    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();
    EXPECT_CALL(*semaphore_mock_, sem_timedwait_monotonic).WillRepeatedly(Return(SemError(ETIMEDOUT)));
    EXPECT_CALL(*semaphore_mock_, sem_post).Times(0);

    EXPECT_EQ(process_launcher_->waitForkRunning(sync, 1ms), OsalReturnType::kTimeout);
}

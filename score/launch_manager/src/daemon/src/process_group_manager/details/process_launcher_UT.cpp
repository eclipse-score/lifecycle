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

#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"

using namespace testing;

// NOLINTBEGIN - clang-tidy does not like syscalls :D

class SyscallMock
{
  public:
    MOCK_METHOD(pid_t, fork, (), ());
    MOCK_METHOD(int, execve, (const char*, char* const[], char* const[]), ());
    MOCK_METHOD(int, kill, (pid_t, int), ());
    MOCK_METHOD(pid_t, wait, (int*), ());
    MOCK_METHOD(int, shm_open, (const char*, int, mode_t), ());
    MOCK_METHOD(int, shm_unlink, (const char*), ());
    MOCK_METHOD(int, ftruncate, (int, off_t), ());
    MOCK_METHOD(int, access, (const char*, int), ());
};

std::unique_ptr<SyscallMock> g_syscall_mock = nullptr;

extern "C" {
// wrap for fork
extern pid_t __real_fork(void);

pid_t __wrap_fork(void)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->fork();
    }

    return __real_fork();
}

// wrap for execve
extern int __real_execve(const char*, char* const[], char* const[]);

int __wrap_execve(const char* filename, char* const argv[], char* const envp[])
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->execve(filename, argv, envp);
    }

    return __real_execve(filename, argv, envp);
}

// wrap for kill
extern int __real_kill(pid_t, int);

int __wrap_kill(pid_t pid, int sig)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->kill(pid, sig);
    }

    return __real_kill(pid, sig);
}

// wrap for wait
extern pid_t __real_wait(int*);

pid_t __wrap_wait(int* status)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->wait(status);
    }

    return __real_wait(status);
}

// wrap for shm_open
extern int __real_shm_open(const char*, int, mode_t);

int __wrap_shm_open(const char* name, int oflag, mode_t mode)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->shm_open(name, oflag, mode);
    }

    return __real_shm_open(name, oflag, mode);
}

// wrap for shm_unlink
extern int __real_shm_unlink(const char*);

int __wrap_shm_unlink(const char* name)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->shm_unlink(name);
    }

    return __real_shm_unlink(name);
}

// wrap for ftruncate
extern int __real_ftruncate(int fildes, off_t length);

int __wrap_ftruncate(int fildes, off_t length)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->ftruncate(fildes, length);
    }

    return __real_ftruncate(fildes, length);
}

// wrap for access
extern int __real_access(const char* name, int type);

int __wrap_access(const char* name, int type)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->access(name, type);
    }

    return __real_access(name, type);
}
}

// NOLINTEND

using namespace score::mw::lifecycle::internal::osal;

class ProcessLauncherTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        g_syscall_mock = std::make_unique<SyscallMock>();

        // Used by logging framework
        EXPECT_CALL(*g_syscall_mock, access).WillRepeatedly(Invoke(__real_access));
    }

    void TearDown() override
    {
        g_syscall_mock.reset();
    }

    ProcessLauncher process_launcher{};
};

TEST_F(ProcessLauncherTest, waitForTerminationSuccess)
{
    RecordProperty("Description", "Test that waitForTermination calls `wait` and sets the correct values");

    const pid_t pid = 7;
    const uint32_t status = 9;

    EXPECT_CALL(*g_syscall_mock, wait).WillOnce(DoAll(SetArgPointee<0>(status), Return(pid)));

    ProcessID out_pid;
    int32_t out_status;
    const auto res = process_launcher.waitForTermination(out_pid, out_status);

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
    const auto res = process_launcher.waitForTermination(out_pid, out_status);

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

    const OsalReturnType res = process_launcher.requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kSuccess);
}

TEST_F(TerminationTest, requestTerminationFailure)
{
    RecordProperty(
        "Description", "Test that requestTermination returns the correct osal result when the `kill` syscall fails");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill).WillOnce(SetErrnoAndReturn(ESRCH, -1));

    const OsalReturnType res = process_launcher.requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, requestTerminationInvalid)
{
    RecordProperty("Description", "Test that requestTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher.requestTermination(pid);

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

    const OsalReturnType res = process_launcher.forceTermination(pid);

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

    const OsalReturnType res = process_launcher.forceTermination(pid);

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

    const OsalReturnType res = process_launcher.forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, forceTerminationInvalid)
{
    RecordProperty("Description", "Test that forceTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher.forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

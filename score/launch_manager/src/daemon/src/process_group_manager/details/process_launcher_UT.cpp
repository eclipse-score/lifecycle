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
    MOCK_METHOD(void*, mmap, (void* __addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset), ());
    MOCK_METHOD(int, munmap, (void* __addr, size_t __len), ());
    MOCK_METHOD(void, sysexit, (int status), ());
    MOCK_METHOD(int, setpgid, (__pid_t __pid, __pid_t __pgid), ());
    MOCK_METHOD(int, setgid, (__gid_t __gid), ());
    MOCK_METHOD(int, setuid, (__uid_t __uid), ());
    MOCK_METHOD(int, sched_setscheduler, (__pid_t __pid, int __policy, const struct sched_param*), ());
    MOCK_METHOD(int, chdir, (const char* __path), ());
    MOCK_METHOD(int, setrlimit, (__rlimit_resource_t __resource, const struct rlimit* __rlimits), ());
    MOCK_METHOD(int, setSecurityPolicy, (const char* policy), ());
    MOCK_METHOD(__pid_t, getpid, (), ());
    MOCK_METHOD(int, fcntl, (int __fd, int __cmd), ());
    MOCK_METHOD(int, setgroups, (size_t n, const gid_t* groups), ());
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

// wrap for mmap
extern void* __real_mmap(void* __addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset);

void* __wrap_mmap(void* __addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->mmap(__addr, __len, __prot, __flags, __fd, __offset);
    }

    return __real_mmap(__addr, __len, __prot, __flags, __fd, __offset);
}

// wrap for munmap
extern int __real_munmap(void* __addr, size_t __len);

int __wrap_munmap(void* __addr, size_t __len)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->munmap(__addr, __len);
    }

    return __real_munmap(__addr, __len);
}

// wrap for setpgid
extern int __real_setpgid(__pid_t __pid, __pid_t __pgid);

int __wrap_setpgid(__pid_t __pid, __pid_t __pgid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setpgid(__pid, __pgid);
    }

    return __real_setpgid(__pid, __pgid);
}

// wrap for setgid
extern int __real_setgid(__gid_t __gid);

int __wrap_setgid(__gid_t __gid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setgid(__gid);
    }

    return __real_setgid(__gid);
}

// wrap for setuid
extern int __real_setuid(__uid_t __uid);

int __wrap_setuid(__uid_t __uid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setuid(__uid);
    }

    return __real_setuid(__uid);
}

// wrap for sched_setscheduler
extern int __real_sched_setscheduler(__pid_t __pid, int __policy, const struct sched_param* __param);

int __wrap_sched_setscheduler(__pid_t __pid, int __policy, const struct sched_param* __param)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->sched_setscheduler(__pid, __policy, __param);
    }

    return __real_sched_setscheduler(__pid, __policy, __param);
}

// wrap for chdir
extern int __real_chdir(const char* __path);

int __wrap_chdir(const char* __path)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->chdir(__path);
    }

    return __real_chdir(__path);
}

// wrap for setrlimit
extern int __real_setrlimit(__rlimit_resource_t __resource, const struct rlimit* __rlimits);

int __wrap_setrlimit(__rlimit_resource_t __resource, const struct rlimit* __rlimits)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setrlimit(__resource, __rlimits);
    }

    return __real_setrlimit(__resource, __rlimits);
}

// wrap for getpid
extern int __real_getpid();

__pid_t __wrap_getpid()
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->getpid();
    }

    return __real_getpid();
}

// wrap for fcntl
extern int __real_fcntl(int __fd, int __cmd, ...);

int __wrap_fcntl(int __fd, int __cmd, ...)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->fcntl(__fd, __cmd);
    }

    return __real_fcntl(__fd, __cmd);
}
}

namespace score::mw::lifecycle::internal::osal
{
void sysexit(int status)
{
    g_syscall_mock->sysexit(status);
}

int setSecurityPolicy(const char* policy)
{
    return g_syscall_mock->setSecurityPolicy(policy);
}

int setgroups(size_t n, const gid_t* groups)
{
    return g_syscall_mock->setgroups(n, groups);
}

}  // namespace score::mw::lifecycle::internal::osal

// NOLINTEND

using namespace score::mw::lifecycle::internal::osal;
using namespace score::mw::lifecycle::internal;

class ProcessLauncherTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        g_syscall_mock = std::make_unique<SyscallMock>();
        process_launcher = std::make_unique<ProcessLauncher>();

        // Used by logging framework
        ON_CALL(*g_syscall_mock, access).WillByDefault(Invoke(__real_access));
    }

    void TearDown() override
    {
        process_launcher.reset();
        g_syscall_mock.reset();
    }

    std::unique_ptr<ProcessLauncher> process_launcher;
};

TEST_F(ProcessLauncherTest, waitForTerminationSuccess)
{
    RecordProperty("Description", "Test that waitForTermination calls `wait` and sets the correct values");

    const pid_t pid = 7;
    const uint32_t status = 9;

    EXPECT_CALL(*g_syscall_mock, wait).WillOnce(DoAll(SetArgPointee<0>(status), Return(pid)));

    ProcessID out_pid;
    int32_t out_status;
    const auto res = process_launcher->waitForTermination(out_pid, out_status);

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
    const auto res = process_launcher->waitForTermination(out_pid, out_status);

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

    const OsalReturnType res = process_launcher->requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kSuccess);
}

TEST_F(TerminationTest, requestTerminationFailure)
{
    RecordProperty(
        "Description", "Test that requestTermination returns the correct osal result when the `kill` syscall fails");
    const pid_t pid = 7;

    EXPECT_CALL(*g_syscall_mock, kill).WillOnce(SetErrnoAndReturn(ESRCH, -1));

    const OsalReturnType res = process_launcher->requestTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, requestTerminationInvalid)
{
    RecordProperty("Description", "Test that requestTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher->requestTermination(pid);

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

    const OsalReturnType res = process_launcher->forceTermination(pid);

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

    const OsalReturnType res = process_launcher->forceTermination(pid);

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

    const OsalReturnType res = process_launcher->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

TEST_F(TerminationTest, forceTerminationInvalid)
{
    RecordProperty("Description", "Test that forceTermination rejects invalid PIDs");

    const pid_t pid = -1;

    EXPECT_CALL(*g_syscall_mock, kill).Times(0);

    const OsalReturnType res = process_launcher->forceTermination(pid);

    EXPECT_EQ(res, OsalReturnType::kFail);
}

class StartProcessTest : public ProcessLauncherTest
{
  protected:
    void SetUp() override
    {
        ProcessLauncherTest::SetUp();

        config_.name = "TestComponent";
        config_.component_properties.binary_name = "TestProcess";
        config_.component_properties.application_profile.application_type = configuration::ApplicationType::Native;
        config_.deployment_config.bin_dir = "/bin";
        config_.deployment_config.working_dir = "/tmp";
        config_.deployment_config.sandbox.max_memory_usage = std::nullopt;
        config_.deployment_config.sandbox.max_cpu_usage = std::nullopt;
        config_.deployment_config.sandbox.max_memory_usage = std::nullopt;
        config_.deployment_config.sandbox.security_policy = std::nullopt;
        config_.deployment_config.sandbox.scheduling_priority = 1;
        config_.deployment_config.sandbox.scheduling_policy = SCHED_RR;
        config_.deployment_config.sandbox.gid = 1;
        config_.deployment_config.sandbox.uid = 1;
        config_.deployment_config.sandbox.supplementary_group_ids = {};
    }

    void TearDown() override
    {
        sync_.reset();

        ProcessLauncherTest::TearDown();
    }

    IpcCommsSync* StubShmObject()
    {
        const int fd = 123;
        void* data = static_cast<void*>(test_buffer.data());
        EXPECT_CALL(*g_syscall_mock, shm_open).WillOnce(Return(fd));
        EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, fd, _)).WillOnce(Return(data));
        EXPECT_CALL(*g_syscall_mock, ftruncate(fd, _)).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, munmap(data, sizeof(IpcCommsSync))).WillOnce(Return(0));

        return static_cast<IpcCommsSync*>(data);
    }

    configuration::ComponentConfig config_ = {};
    ProcessID pid_;
    IpcCommsP sync_;

    alignas(IpcCommsSync) std::array<std::byte, sizeof(IpcCommsSync)> test_buffer;
};

TEST_F(StartProcessTest, startProcessNoFile)
{
    RecordProperty("Description", "Test that startProcess returns a failure if the provided path does not exist");

    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(StartProcessTest, startProcessNoPath)
{
    RecordProperty("Description", "Test that startProcess returns a failure if the provided path is empty");

    config_.component_properties.binary_name = "";
    EXPECT_CALL(*g_syscall_mock, access).Times(0);  // Access should not be called on an empty path

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(StartProcessTest, startProcessForkFailed)
{
    RecordProperty("Description", "Test that startProcess returns a failure if the provided path is empty");

    EXPECT_CALL(*g_syscall_mock, access(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, fork).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

class SetSchedulingAndSecurityTest : public StartProcessTest
{
  protected:
    void SetUp() override
    {
        StartProcessTest::SetUp();

        EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, fork).WillOnce(Return(0));  // Test the child process
    }
};

TEST_F(SetSchedulingAndSecurityTest, setpgidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting pgid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setgidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting gid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setuidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting uid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setschedFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting the scheduler fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setschedClampedUpper)
{
    RecordProperty(
        "Description", "Verify that if the configured priority is higher than the OS supports, it is clamped");

    config_.deployment_config.sandbox.scheduling_policy = SCHED_FIFO;
    const int too_high = 1000;
    config_.deployment_config.sandbox.scheduling_priority = too_high;

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sched_setscheduler(_, _, Field(&sched_param::sched_priority, Lt(too_high))))
        .WillOnce(Return(0));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setschedClampedLower)
{
    RecordProperty(
        "Description", "Verify that if the configured priority is lower than the OS supports, it is clamped");

    config_.deployment_config.sandbox.scheduling_policy = SCHED_FIFO;
    const int too_low = -10;
    config_.deployment_config.sandbox.scheduling_priority = too_low;

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sched_setscheduler(_, _, Field(&sched_param::sched_priority, Gt(too_low))))
        .WillOnce(Return(0));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(SetSchedulingAndSecurityTest, setgroupsFails)
{
    RecordProperty("Description", "Verify that if setting supplementary gids fails, the forked process exits");

    config_.deployment_config.sandbox.supplementary_group_ids = {1, 2, 3};

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgroups).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(StartProcessTest, chdirFails)
{
    RecordProperty("Description", "Verify that the forked process exits if changing the working dir fails");

    EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, chdir).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(StartProcessTest, changeSecurityPolicyFails)
{
    RecordProperty("Description", "Verify that the forked process exits if changing the security policy fails");

    config_.deployment_config.sandbox.security_policy = "security";

    EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setSecurityPolicy).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(StartProcessTest, setRLimitFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting a limit fails");

    config_.deployment_config.sandbox.max_cpu_usage = 500;

    EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setrlimit).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(StartProcessTest, execveFails)
{
    RecordProperty("Description", "Verify that the forked process exits if execve fails");

    EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, execve).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

// Define a matcher that checks a null-terminated char** against a vector of strings
MATCHER_P(ArgvMatches, expected_args, "")
{
    for (size_t i = 0; i < expected_args.size(); ++i)
    {
        // Check if the argument is null prematurely, or if the string doesn't match
        if (arg[i] == nullptr || std::string(arg[i]) != expected_args[i])
        {
            *result_listener << "mismatch at index " << i << " (expected: \"" << expected_args[i] << "\", got: \""
                             << (arg[i] ? arg[i] : "NULL") << "\")";
            return false;
        }
    }
    // execve argv must be null-terminated; ensure the last expected element is followed by NULL
    return arg[expected_args.size()] == nullptr;
}

MATCHER_P(CArrayMatches, expected, "")
{
    for (size_t i = 0; i < expected.size(); ++i)
    {
        if (arg[i] != expected[i])
        {
            *result_listener << "mismatch at index " << i << " (expected: " << expected[i] << ", got: " << arg[i]
                             << ")";
            return false;
        }
    }
    return true;
}

TEST_F(StartProcessTest, startProcessChildSuccess)
{
    RecordProperty("Description", "Verify that when startProcess succeeds, the forked process is configured correctly");

    const pid_t forked_pid = 23;
    const int scheduler = SCHED_FIFO;
    const int uid = 11;
    const int gid = 13;
    const std::vector<gid_t> sgids = {1, 2, 3};
    const std::uint64_t mem_limit = 4096;
    const std::uint32_t cpu_limit = 500;
    const std::string security_policy = "security";
    const std::vector<std::string> args_in = {"-c 2", "--argument yes"};
    const std::vector<std::string> expected_launch_args = {"/bin/TestProcess", args_in[0], args_in[1]};

    config_.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;
    config_.deployment_config.sandbox.scheduling_policy = scheduler;
    config_.deployment_config.sandbox.uid = uid;
    config_.deployment_config.sandbox.gid = gid;
    config_.deployment_config.sandbox.supplementary_group_ids = sgids;
    config_.deployment_config.sandbox.max_memory_usage = mem_limit;
    config_.deployment_config.sandbox.max_cpu_usage = cpu_limit;
    config_.deployment_config.sandbox.security_policy = security_policy;
    config_.deployment_config.environmental_variables.add("environment", "yes");
    config_.deployment_config.environmental_variables.add("errors", "no");
    config_.component_properties.process_arguments = args_in;

    IpcCommsSync* block = StubShmObject();

    EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, fork).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, getpid).WillRepeatedly(Return(forked_pid));
    EXPECT_CALL(*g_syscall_mock, fcntl).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setpgid(0, forked_pid));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler(0, scheduler, _)).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid(uid)).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid(gid)).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgroups(sgids.size(), CArrayMatches(sgids))).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, chdir(StrEq("/tmp"))).WillOnce(Return(0));
    EXPECT_CALL(
        *g_syscall_mock,
        setrlimit(RLIMIT_DATA, AllOf(Field(&rlimit::rlim_cur, mem_limit), Field(&rlimit::rlim_max, mem_limit))));
    EXPECT_CALL(
        *g_syscall_mock,
        setrlimit(RLIMIT_AS, AllOf(Field(&rlimit::rlim_cur, mem_limit), Field(&rlimit::rlim_max, mem_limit))));
    EXPECT_CALL(
        *g_syscall_mock,
        setrlimit(RLIMIT_CPU, AllOf(Field(&rlimit::rlim_cur, cpu_limit), Field(&rlimit::rlim_max, cpu_limit))));
    EXPECT_CALL(*g_syscall_mock, setSecurityPolicy(StrEq(security_policy.data()))).WillOnce(Return(0));
    EXPECT_CALL(
        *g_syscall_mock,
        execve(
            StrEq(expected_launch_args[0]),
            ArgvMatches(expected_launch_args),
            config_.deployment_config.environmental_variables.envp()))
        .WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit).Times(0);  // No failures

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process

    EXPECT_EQ(block->pid_, forked_pid);
    EXPECT_EQ(block->comms_type_, CommsType::kReporting);
}

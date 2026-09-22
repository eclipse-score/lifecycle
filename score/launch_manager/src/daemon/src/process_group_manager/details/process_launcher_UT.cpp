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

#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"

using namespace testing;

// NOLINTBEGIN - clang-tidy does not like syscalls

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
    MOCK_METHOD(int, fcntl, (int __fd, int __cmd, void* arg), ());
    MOCK_METHOD(int, setgroups, (size_t n, const gid_t* groups), ());
    MOCK_METHOD(int, sem_init, (sem_t * __sem, int __pshared, unsigned int __value), ());
    MOCK_METHOD(int, sem_destroy, (sem_t * __sem), ());
    MOCK_METHOD(int, sem_trywait, (sem_t * __sem), ());
    MOCK_METHOD(int, sem_post, (sem_t * __sem), ());
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
    void* arg = nullptr;
    va_list args;

    va_start(args, __cmd);
    arg = va_arg(args, void*);
    va_end(args);

    if (g_syscall_mock)
    {
        return g_syscall_mock->fcntl(__fd, __cmd, arg);
    }

    return __real_fcntl(__fd, __cmd, arg);
}

// wrap for sem_init
extern int __real_sem_init(sem_t* __sem, int __pshared, unsigned int __value);

int __wrap_sem_init(sem_t* __sem, int __pshared, unsigned int __value)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->sem_init(__sem, __pshared, __value);
    }

    return __real_sem_init(__sem, __pshared, __value);
}

// wrap for sem_destroy
extern int __real_sem_destroy(sem_t* __sem);

int __wrap_sem_destroy(sem_t* __sem)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->sem_destroy(__sem);
    }

    return __real_sem_destroy(__sem);
}

// wrap for sem_trywait
extern int __real_sem_trywait(sem_t* __sem);

int __wrap_sem_trywait(sem_t* __sem)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->sem_trywait(__sem);
    }

    return __real_sem_trywait(__sem);
}

// wrap for sem_post
extern int __real_sem_post(sem_t* __sem);

int __wrap_sem_post(sem_t* __sem)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->sem_post(__sem);
    }

    return __real_sem_post(__sem);
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
        ON_CALL(*g_syscall_mock, fcntl).WillByDefault(Invoke(__real_fcntl));
        EXPECT_CALL(*g_syscall_mock, access).Times(AnyNumber());
        EXPECT_CALL(*g_syscall_mock, fcntl).Times(AnyNumber());
    }

    void TearDown() override
    {
        process_launcher.reset();
        g_syscall_mock.reset();
    }

    std::shared_ptr<IpcCommsSync> GetInitialisedIpc()
    {
        std::memset(test_buffer.data(), 0, test_buffer.size());
        auto* sync = reinterpret_cast<IpcCommsSync*>(test_buffer.data());

        sync->comms_type_ = score::mw::lifecycle::internal::osal::CommsType::kNoComms;
        sync->pid_ = 0;
        EXPECT_EQ(sync->reply_sync_.init(0, false), OsalReturnType::kSuccess);
        EXPECT_EQ(sync->send_sync_.init(0, false), OsalReturnType::kSuccess);

        std::shared_ptr<IpcCommsSync> shared{sync, [](IpcCommsSync* ptr) {
                                                 EXPECT_EQ(ptr->reply_sync_.deinit(), OsalReturnType::kSuccess);
                                                 EXPECT_EQ(ptr->send_sync_.deinit(), OsalReturnType::kSuccess);
                                             }};

        return shared;
    }

    void UseRealSemaphores()
    {
        ON_CALL(*g_syscall_mock, sem_init).WillByDefault(Invoke(__real_sem_init));
        ON_CALL(*g_syscall_mock, sem_destroy).WillByDefault(Invoke(__real_sem_destroy));
        ON_CALL(*g_syscall_mock, sem_trywait).WillByDefault(Invoke(__real_sem_trywait));
        ON_CALL(*g_syscall_mock, sem_post).WillByDefault(Invoke(__real_sem_post));
        EXPECT_CALL(*g_syscall_mock, sem_init).Times(AtLeast(1));
        EXPECT_CALL(*g_syscall_mock, sem_destroy).Times(AtLeast(1));
        EXPECT_CALL(*g_syscall_mock, sem_trywait).Times(AnyNumber());
        EXPECT_CALL(*g_syscall_mock, sem_post).Times(AnyNumber());
    }

    std::unique_ptr<ProcessLauncher> process_launcher;

    alignas(IpcCommsSync) std::array<std::byte, sizeof(IpcCommsSync)> test_buffer;
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

// The sysexit mock can throw this to interrupt execution
struct SysExitException
{
};

class StartProcessTest : public ProcessLauncherTest
{
  protected:
    void SetUp() override
    {
        ProcessLauncherTest::SetUp();

        EXPECT_CALL(*g_syscall_mock, getpid).Times(AnyNumber()).WillRepeatedly(Return(forked_pid));

        config_.name = "TestComponent";
        config_.component_properties.binary_name = "TestProcess";
        config_.component_properties.application_profile.application_type = configuration::ApplicationType::Native;
        config_.deployment_config.executable_path = "/bin/TestProcess";
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

    void ExpectChildProcessStarts()
    {
        EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, fork).WillOnce(Return(0));
    }

    void ExpectSuccessfulSchedulingAndSecurity()
    {
        EXPECT_CALL(*g_syscall_mock, setpgid).Times(AnyNumber()).WillRepeatedly(Return(0));
        EXPECT_CALL(*g_syscall_mock, sched_setscheduler).Times(AnyNumber()).WillRepeatedly(Return(0));
        EXPECT_CALL(*g_syscall_mock, setgid).Times(AnyNumber()).WillRepeatedly(Return(0));
        EXPECT_CALL(*g_syscall_mock, setuid).Times(AnyNumber()).WillRepeatedly(Return(0));
    }

    void ExpectSuccessfulChdir()
    {
        EXPECT_CALL(*g_syscall_mock, chdir).Times(AnyNumber()).WillRepeatedly(Return(0));
    }

    IpcCommsSync* StubShmObject()
    {
        const int fd = 123;
        void* data = static_cast<void*>(test_buffer.data());
        EXPECT_CALL(*g_syscall_mock, shm_open).WillOnce(Return(fd));
        EXPECT_CALL(*g_syscall_mock, shm_unlink).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, mmap(_, _, _, _, fd, _)).WillOnce(Return(data));
        EXPECT_CALL(*g_syscall_mock, ftruncate(fd, _)).WillOnce(Return(0));
        EXPECT_CALL(*g_syscall_mock, munmap(data, sizeof(IpcCommsSync))).WillOnce(Return(0));

        return static_cast<IpcCommsSync*>(data);
    }

    void ExpectSetupComms()
    {
        StubShmObject();

        EXPECT_CALL(*g_syscall_mock, sem_init).Times(AnyNumber()).WillRepeatedly(Return(0));
    }

    const pid_t forked_pid = 23;

    configuration::ComponentConfig config_ = {};
    ProcessID pid_;
    IpcCommsP sync_;
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

TEST_F(StartProcessTest, handleCommsFailsFnctl)
{
    RecordProperty(
        "Description", "Verify that the forked process exits if setting up comms fails due to a syscall failure");

    config_.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;
    ExpectChildProcessStarts();
    ExpectSetupComms();
    EXPECT_CALL(*g_syscall_mock, fcntl).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

class SetupCommsTest : public StartProcessTest
{
    void SetUp() override
    {
        StartProcessTest::SetUp();

        config_.component_properties.application_profile.application_type = configuration::ApplicationType::Reporting;

        EXPECT_CALL(*g_syscall_mock, access).WillOnce(Return(0));
    }
};

TEST_F(SetupCommsTest, shmOpenFails)
{
    RecordProperty("Description", "Verify that if opening shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, ftruncateFails)
{
    RecordProperty("Description", "Verify that if truncating shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open).WillOnce(Return(123));
    EXPECT_CALL(*g_syscall_mock, shm_unlink).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, ftruncate).WillOnce(Return(-1));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, getCommsFails)
{
    RecordProperty("Description", "Verify that if truncating shared memory fails, startProcess fails");

    EXPECT_CALL(*g_syscall_mock, shm_open).WillOnce(Return(123));
    EXPECT_CALL(*g_syscall_mock, shm_unlink).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, ftruncate).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, mmap).WillOnce(Return(MAP_FAILED));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

TEST_F(SetupCommsTest, initSemaphoresFails)
{
    RecordProperty("Description", "Verify that if setting up the semaphores fails, startProcess fails");

    StubShmObject();
    EXPECT_CALL(*g_syscall_mock, sem_init).WillRepeatedly(Return(-1));

    EXPECT_EQ(process_launcher->startProcess(pid_, sync_, config_), OsalReturnType::kFail);
}

class SetSchedulingAndSecurityTest : public StartProcessTest
{
  protected:
    void SetUp() override
    {
        StartProcessTest::SetUp();

        ExpectChildProcessStarts();
        ExpectSuccessfulChdir();
        EXPECT_CALL(*g_syscall_mock, execve).Times(AtMost(1));
    }
};

TEST_F(SetSchedulingAndSecurityTest, setpgidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting pgid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(SetSchedulingAndSecurityTest, setgidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting gid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(SetSchedulingAndSecurityTest, setuidFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting uid fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(SetSchedulingAndSecurityTest, setschedFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting the scheduler fails");

    EXPECT_CALL(*g_syscall_mock, setpgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, sched_setscheduler).WillOnce(Return(-1));
    EXPECT_CALL(*g_syscall_mock, setgid).WillOnce(Return(0));
    EXPECT_CALL(*g_syscall_mock, setuid).WillOnce(Return(0));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
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

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(StartProcessTest, chdirFails)
{
    RecordProperty("Description", "Verify that the forked process exits if changing the working dir fails");

    ExpectChildProcessStarts();
    ExpectSuccessfulSchedulingAndSecurity();
    EXPECT_CALL(*g_syscall_mock, chdir).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(StartProcessTest, changeSecurityPolicyFails)
{
    RecordProperty("Description", "Verify that the forked process exits if changing the security policy fails");

    config_.deployment_config.sandbox.security_policy = "security";

    ExpectChildProcessStarts();
    ExpectSuccessfulSchedulingAndSecurity();
    ExpectSuccessfulChdir();
    EXPECT_CALL(*g_syscall_mock, setSecurityPolicy).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(StartProcessTest, setRLimitFails)
{
    RecordProperty("Description", "Verify that the forked process exits if setting a limit fails");

    config_.deployment_config.sandbox.max_cpu_usage = 500;

    ExpectChildProcessStarts();
    ExpectSuccessfulSchedulingAndSecurity();
    ExpectSuccessfulChdir();
    EXPECT_CALL(*g_syscall_mock, setrlimit).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
}

TEST_F(StartProcessTest, setRLimitIgnore)
{
    RecordProperty("Description", "Verify that rlimits are not set if the configured value is 0");

    config_.deployment_config.sandbox.max_cpu_usage = 0;
    config_.deployment_config.sandbox.max_memory_usage = 0;

    ExpectChildProcessStarts();
    ExpectSuccessfulSchedulingAndSecurity();
    ExpectSuccessfulChdir();
    EXPECT_CALL(*g_syscall_mock, setrlimit).Times(0);
    EXPECT_CALL(*g_syscall_mock, execve).Times(AtMost(1));

    static_cast<void>(process_launcher->startProcess(pid_, sync_, config_));  // No return from forked process
}

TEST_F(StartProcessTest, execveFails)
{
    RecordProperty("Description", "Verify that the forked process exits if execve fails");

    ExpectChildProcessStarts();
    ExpectSuccessfulSchedulingAndSecurity();
    ExpectSuccessfulChdir();
    EXPECT_CALL(*g_syscall_mock, execve).WillOnce(Return(-1));

    EXPECT_CALL(*g_syscall_mock, sysexit(EXIT_FAILURE)).WillOnce(Throw(SysExitException{}));

    EXPECT_THROW(static_cast<void>(process_launcher->startProcess(pid_, sync_, config_)), SysExitException);
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

    const int scheduler = SCHED_FIFO;
    const int uid = 11;
    const int gid = 13;
    const std::vector<gid_t> sgids = {1, 2, 3};
    const std::uint64_t mem_limit = 4096;
    const std::uint32_t cpu_limit = 500;
    const std::string security_policy = "security";
    const std::vector<std::string> args_in = {"-c 2", "--argument yes"};
    const std::vector<std::string> expected_launch_args = {
        config_.deployment_config.executable_path, args_in[0], args_in[1]};

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
    EXPECT_CALL(*g_syscall_mock, sem_init).Times(2).WillRepeatedly(Return(0));
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

class ClientMethodsTest : public ProcessLauncherTest
{
};

using namespace std::chrono_literals;

TEST_F(ProcessLauncherTest, ignoreRunningNoPost)
{
    RecordProperty("Description", "Verify that ignoreRunning correctly handles a failed semaphore post");

    EXPECT_CALL(*g_syscall_mock, sem_init).WillRepeatedly(Return(0));
    EXPECT_CALL(*g_syscall_mock, sem_destroy).WillRepeatedly(Return(0));
    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();
    EXPECT_CALL(*g_syscall_mock, sem_post).WillOnce(SetErrnoAndReturn(EINVAL, -1));

    EXPECT_EQ(process_launcher->waitForkRunning(sync, std::nullopt), OsalReturnType::kFail);
}

TEST_F(ProcessLauncherTest, ignoreRunningSuccess)
{
    RecordProperty(
        "Description",
        "Verify that waitForkRunning without a timeout posts on the reply semaphore without waiting and returns a "
        "success");

    UseRealSemaphores();
    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();
    OsalReturnType waitRes = OsalReturnType::kFail;

    auto waiter = std::thread{[&waitRes, &sync]() {
        waitRes = sync->reply_sync_.timedWait(5000ms);
    }};

    EXPECT_EQ(process_launcher->waitForkRunning(sync, std::nullopt), OsalReturnType::kSuccess);
    waiter.join();
    EXPECT_EQ(waitRes, OsalReturnType::kSuccess);
}

TEST_F(ProcessLauncherTest, kRunningNoSync)
{
    RecordProperty("Description", "Verify that waitForkRunning correctly handles a null pointer");

    std::shared_ptr<IpcCommsSync> sync;

    EXPECT_EQ(process_launcher->waitForkRunning(sync, 1ms), OsalReturnType::kFail);
}

TEST_F(ProcessLauncherTest, kRunningSuccess)
{
    RecordProperty(
        "Description",
        "Verify that waitForkRunning waits for a notification, posts a reply, and then waits for another notification "
        "before proceeding");

    UseRealSemaphores();
    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();
    OsalReturnType waitRes = OsalReturnType::kFail;
    OsalReturnType postRes = OsalReturnType::kFail;

    auto waiter = std::thread{[&waitRes, &postRes, sync]() {
        postRes = sync->send_sync_.post();
        if (postRes == OsalReturnType::kSuccess)
        {
            waitRes = sync->reply_sync_.timedWait(5000ms);
        }
        if (waitRes == OsalReturnType::kSuccess)
        {
            postRes = sync->send_sync_.post();
        }
    }};

    EXPECT_EQ(process_launcher->waitForkRunning(sync, 5000ms), OsalReturnType::kSuccess);
    waiter.join();
    ASSERT_EQ(postRes, OsalReturnType::kSuccess) << "Posting on the semaphore failed (test problem)";
    EXPECT_EQ(waitRes, OsalReturnType::kSuccess);
}

TEST_F(ProcessLauncherTest, kRunningTimeout)
{
    RecordProperty(
        "Description",
        "Verify that waitForkRunning returns a timeout failure if no notification is received within the timeout");

    UseRealSemaphores();

    std::shared_ptr<IpcCommsSync> sync = GetInitialisedIpc();

    EXPECT_EQ(process_launcher->waitForkRunning(sync, 1ms), OsalReturnType::kTimeout);
}

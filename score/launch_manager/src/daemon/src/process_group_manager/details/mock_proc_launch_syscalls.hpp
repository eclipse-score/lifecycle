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

#ifndef MOCK_PROC_LAUNCH_SYSCALLS
#define MOCK_PROC_LAUNCH_SYSCALLS

#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gmock/gmock.h>

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
    MOCK_METHOD(void*, mmap, (void* __addr, size_t __len, int __prot, int __flags, int __fd, off_t __offset), ());
    MOCK_METHOD(int, munmap, (void* __addr, size_t __len), ());
    MOCK_METHOD(void, sysexit, (int status), ());
    MOCK_METHOD(int, setpgid, (pid_t __pid, pid_t __pgid), ());
    MOCK_METHOD(int, setgid, (gid_t __gid), ());
    MOCK_METHOD(int, setuid, (uid_t __uid), ());
    MOCK_METHOD(int, sched_setscheduler, (pid_t __pid, int __policy, const struct sched_param*), ());
    MOCK_METHOD(int, chdir, (const char* __path), ());
    MOCK_METHOD(int, setrlimit, (int __resource, const struct rlimit* __rlimits), ());
    MOCK_METHOD(int, setSecurityPolicy, (const char* policy), ());
    MOCK_METHOD(pid_t, getpid, (), ());
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
extern void* __real_mmap(void* __addr, size_t __len, int __prot, int __flags, int __fd, off_t __offset);

void* __wrap_mmap(void* __addr, size_t __len, int __prot, int __flags, int __fd, off_t __offset)
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
extern int __real_setpgid(pid_t __pid, pid_t __pgid);

int __wrap_setpgid(pid_t __pid, pid_t __pgid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setpgid(__pid, __pgid);
    }

    return __real_setpgid(__pid, __pgid);
}

// wrap for setgid
extern int __real_setgid(gid_t __gid);

int __wrap_setgid(gid_t __gid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setgid(__gid);
    }

    return __real_setgid(__gid);
}

// wrap for setuid
extern int __real_setuid(uid_t __uid);

int __wrap_setuid(uid_t __uid)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setuid(__uid);
    }

    return __real_setuid(__uid);
}

// wrap for sched_setscheduler
extern int __real_sched_setscheduler(pid_t __pid, int __policy, const struct sched_param* __param);

int __wrap_sched_setscheduler(pid_t __pid, int __policy, const struct sched_param* __param)
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
extern int __real_setrlimit(int __resource, const struct rlimit* __rlimits);

int __wrap_setrlimit(int __resource, const struct rlimit* __rlimits)
{
    if (g_syscall_mock)
    {
        return g_syscall_mock->setrlimit(__resource, __rlimits);
    }

    return __real_setrlimit(__resource, __rlimits);
}

// wrap for getpid
extern int __real_getpid();

pid_t __wrap_getpid()
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

#endif

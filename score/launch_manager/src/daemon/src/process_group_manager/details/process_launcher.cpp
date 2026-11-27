/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <string_view>

#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/common/signal_safe_log.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/configuration/environment_config.hpp"
#include "score/mw/launch_manager/configuration/process_command.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/security_policy.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/mw/launch_manager/osal/set_affinity.hpp"
#include "score/mw/launch_manager/osal/set_groups.hpp"
#include "score/mw/launch_manager/osal/shm_open_anon.hpp"
#include "score/mw/launch_manager/osal/sys_exit.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <score/assert.hpp>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

namespace
{

using score::mw::lifecycle::internal::signal_safe_log;
using score::mw::lifecycle::internal::signal_safe_log_errno;
using score::mw::lifecycle::internal::osal::IpcCommsSync;
using score::mw::lifecycle::internal::osal::sysexit;

/// @brief Applies the given limit.
/// @details The implementation should be async signal safe.
/// @warning This will sysexit if the set is not succesful.
void applyLimitOrDie(const int resource, const rlimit& limit, const std::string_view rlimit_name) noexcept(false)
{
    if (::setrlimit(resource, &limit) == -1)
    {
        static_cast<void>(signal_safe_log_errno(errno, "Failed to apply rlimit ", rlimit_name));
        sysexit(EXIT_FAILURE);
    }
}

/// @brief Returns the RLIMIT_* macro name for the given platform resource constant, for use in
/// log messages only.
std::string_view rlimitResourceName(const int resource) noexcept
{
    switch (resource)
    {
        case RLIMIT_AS:
            return "RLIMIT_AS";
        case RLIMIT_CORE:
            return "RLIMIT_CORE";
        case RLIMIT_CPU:
            return "RLIMIT_CPU";
        case RLIMIT_DATA:
            return "RLIMIT_DATA";
        case RLIMIT_FSIZE:
            return "RLIMIT_FSIZE";
        case RLIMIT_NOFILE:
            return "RLIMIT_NOFILE";
        case RLIMIT_STACK:
            return "RLIMIT_STACK";
        default:
            return "RLIMIT_<unknown>";
    }
}

/// @details The implementation should be async signal safe.
void handleComms(score::mw::lifecycle::internal::osal::ChildProcessConfig& param)
{
    if (!param.shared_block)
    {
        // Native process without comms, fds are CLOEXEC
        return;
    }

    param.fd = dup2(param.fd, param.shared_block->sync_fd);

    // Flag FD_CLOEXEC is cleared to ensure that sync_fd remains open after the execve call.
    if (-1 == fcntl(IpcCommsSync::sync_fd, F_SETFD, 0))
    {
        static_cast<void>(signal_safe_log_errno(errno, "fcntl at line ", __LINE__, " failed"));
        sysexit(EXIT_FAILURE);
    }
}

/// @details The implementation should be async signal safe.
void changeCurrentWorkingDirectory(const score::mw::lifecycle::internal::configuration::DeploymentConfig& config)
{
    // working_dir is set by configuration, so it should always be valid.
    // If not, chdir will fail anyway and we will log an error and exit.
    if (-1 == chdir(config.working_dir.c_str()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "chdir(", config.working_dir, ") failed."));
        sysexit(EXIT_FAILURE);
    }
}

// req-Id: comp_req__launch_man__process_rlimit_support
/// @brief Applies every configured resource limit.
/// @details The implementation should be async signal safe. Must run before the potential privilege drop in
/// setSchedulingAndSecurity().
void applyResourceLimits(const score::mw::lifecycle::internal::configuration::Sandbox& config)
{
    for (const auto& rlimit_config : config.rlimits)
    {
        applyLimitOrDie(rlimit_config.resource, rlimit_config.limit, rlimitResourceName(rlimit_config.resource));
    }
}

/// @details The implementation should be async signal safe.
void changeSecurityPolicy(const score::mw::lifecycle::internal::configuration::Sandbox& config)
{
    if (config.security_policy.has_value() && !config.security_policy.value().empty())
    {
        if (score::mw::lifecycle::internal::osal::setSecurityPolicy(config.security_policy.value().c_str()) != 0)
        {
            static_cast<void>(
                signal_safe_log_errno(errno, "changeSecurityPolicy(", config.security_policy.value(), ") failed"));
            sysexit(EXIT_FAILURE);
        }
    }
}

/// @brief Clears all signal handlers (where valid) and then unblocks them. This allows the child process to receive
/// signals even before execve() is reached.
void resetSignalHandlersAndUnblock()
{
    // If this static assert fails, pthread_sigmask will fail with EINVAL. See `man sigprocmask`.
    // This is more a porting guard than a real life scenario (libc / kernel mismatch)...
    static_assert(
        sizeof(sigset_t) >= (_NSIG / 8),
        "sigset_t is too small to fulfill the kernel's rt_sigprocmask size requirement");

    // Reset all signal handlers to default except unblockable signals
    struct sigaction act{};
    act.sa_handler = SIG_DFL;

    int err = 0;
    for (int sig = 1; sig < _NSIG; ++sig)
    {
        if (sig == SIGKILL || sig == SIGSTOP)
            continue;
        err |= sigaction(sig, &act, NULL);
    }
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(err == 0, "Failed to reset signal handlers");

    // Unblock all signals by applying an empty mask
    sigset_t mask;
    err = sigemptyset(&mask);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(err == 0, "Failed to build signal mask");

    err = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(err == 0, "Invalid argument(s) to pthread_sigmask");
    static_cast<void>(err);
}

}  // namespace

namespace score::mw::lifecycle::internal::osal
{

OsalReturnType
ProcessLauncher::startProcess(ProcessID& pid, IpcCommsP& block, const configuration::ComponentConfig& config)
{
    OsalReturnType result = OsalReturnType::kFail;

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        !config.deployment_config.process_command.executablePath().empty(), "Executable path must not be empty");

    if (access(config.deployment_config.process_command.executablePath().c_str(), X_OK) != 0)
    {
        static_cast<void>(signal_safe_log(
            "File does not exist or is not executable: ", config.deployment_config.process_command.executablePath()));
        return result;
    }

    int fd = -1;
    pid = -1;
    block = nullptr;
    bool comms_result = true;

    const auto& app_type = config.component_properties.application_profile.application_type;
    if (app_type != configuration::ApplicationType::Native)
    {
        comms_result = setupComms(block, fd, config);
    }

    if (comms_result)
    {
        if (fflush(stdout) == EOF)
        {
            LM_LOG_ERROR() << "Failed to flush stdout before forking.";
        }

        pid = fork();

        if (pid == kPosixSuccess)
        {
            /*
             * From this point on, only async signal safe functions can be
             * used. `fork` only copies the current thread, so any locks
             * which were held at that time will never be released.
             * See `man 2 fork`.
             */
            ChildProcessConfig param = {config, fd, block};
            handleChildProcess(param);
            result = OsalReturnType::kSuccess;
        }
        else if (pid > kPidZero)
        {
            result = OsalReturnType::kSuccess;
        }
        else
        {
            LM_LOG_ERROR() << "Fork failed: Unable to create a new process.";
        }
    }
    else
    {
        LM_LOG_ERROR() << "Shared memory creation failed: Unable to create shared memory for kRunning communication.";
    }

    if (fd >= 0)
    {
        close(fd);
    }

    return result;
}

bool ProcessLauncher::setupComms(IpcCommsP& block, int& fd, const configuration::ComponentConfig& config)
{
    size_t length = sizeof(IpcCommsSync);

    fd = shm_open_anon("/lm_running_report_ipc\0");
    if (fd < 0)
    {
        LM_LOG_ERROR() << "shm_open failed:" << config.deployment_config.process_command.executablePath()
                       << "Unable to open shared memory object. Error:" << errno_message(errno);
        return false;
    }

    if (-1 == ftruncate(fd, static_cast<int>(length)))
    {
        LM_LOG_ERROR() << "ftruncate failed:" << config.deployment_config.process_command.executablePath()
                       << "Unable to set size of shared memory file descriptor. Error:" << errno_message(errno);
        return false;
    }

    block = IpcCommsSync::createCommsObject(fd);
    if (!block)
    {
        LM_LOG_ERROR() << "Creating comms object failed:" << config.name;
        return false;
    }

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        config.component_properties.application_profile.application_type != configuration::ApplicationType::Native,
        "We should not set up the comms object if the application type is native");

    return true;
}

/// @details The implementation should be async signal safe.
OsalReturnType ProcessLauncher::setSchedulingAndSecurity(const configuration::Sandbox& config)
{
    OsalReturnType retval = OsalReturnType::kSuccess;

    // Set process group id to be equal to the pid
    if (0 != setpgid(0, getpid()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setpgid() failed"));
        retval = OsalReturnType::kFail;
    }
    // Set scheduling policy with sched_setscheduler.
    // The priority was clamped to the valid range of the policy when the configuration was loaded.
    sched_param sch_param{};
    sch_param.sched_priority = config.scheduling_priority;

    if (-1 == sched_setscheduler(0, config.scheduling_policy, &sch_param))
    {
        static_cast<void>(signal_safe_log_errno(errno, "sched_setscheduler() failed"));
        retval = OsalReturnType::kFail;
    }

    // Set core affinity using OS specific functionality in osal
    if (config.affinity_mask.has_value())
    {
        if (-1 == osal::setaffinity(config.affinity_mask.value()))
        {
            static_cast<void>(signal_safe_log_errno(errno, "setaffinity failed"));
            retval = OsalReturnType::kFail;
        }
    }

    // Set group ID
    if (-1 == setgid(config.gid))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setgid(", config.gid, ") failed"));
        retval = OsalReturnType::kFail;
    }
    // Set supplementary group ids
    size_t supplementary_gids_number = config.supplementary_group_ids.size();

    // Note: the type of the first parameter of setgroups() differs in Linux and QNX, so we use osal
    if (-1 == osal::setgroups(supplementary_gids_number, config.supplementary_group_ids.data()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setgroups() failed"));
        retval = OsalReturnType::kFail;
    }

    // Set user ID
    if (-1 == setuid(config.uid))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setuid(", config.uid, ") failed"));
        retval = OsalReturnType::kFail;
    }

    return retval;
}

/// @details The implementation should be async signal safe.
void ProcessLauncher::handleChildProcess(ChildProcessConfig& param)
{
    resetSignalHandlersAndUnblock();

    handleComms(param);

    applyResourceLimits(param.config.deployment_config.sandbox);

    if (OsalReturnType::kSuccess != setSchedulingAndSecurity(param.config.deployment_config.sandbox))
    {
        sysexit(EXIT_FAILURE);
    }

    changeCurrentWorkingDirectory(param.config.deployment_config);
    changeSecurityPolicy(param.config.deployment_config.sandbox);

    const auto& process_command = param.config.deployment_config.process_command;

    // Get envp from Environment - it already provides a null-terminated array
    char* const* envp = param.config.deployment_config.environmental_variables.envp();

    // Finally, execute the process, passing all the arguments and environment variables
    static_cast<void>(execve(process_command.executablePath().c_str(), process_command.argv(), envp));
    // A successful execve replaces the process image; returning here always means failure.
    static_cast<void>(signal_safe_log_errno(
        errno, "execve failed: Unable to execute the ", process_command.executablePath(), " app."));
    sysexit(EXIT_FAILURE);
}

OsalReturnType ProcessLauncher::requestTermination(ProcessID pid)
{
    LM_LOG_DEBUG() << "Request termination received for pid" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero)
    {
        if (kill(pid, SIGTERM) == kPosixSuccess)
        {
            result = OsalReturnType::kSuccess;
        }
        else
        {
            LM_LOG_ERROR() << "SIGTERM failed: Unable to send SIGTERM to process ID" << pid
                           << ". Error:" << errno_message(errno);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType ProcessLauncher::forceTermination(ProcessID pid)
{
    LM_LOG_DEBUG() << "Forced termination received for pid" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero)
    {
        if (kill(pid, SIGKILL) == kPosixSuccess)
        {
            result = OsalReturnType::kSuccess;
        }
        else if (errno == ESRCH)
        {
            LM_LOG_WARN() << "SIGKILL failed: Process is already gone (ESRCH) for process ID" << pid;
        }
        else
        {
            LM_LOG_FATAL() << "SIGKILL failed: Unable to send SIGKILL to process ID" << pid;
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType ProcessLauncher::waitForTermination(osal::ProcessID& pid, int32_t& status)
{
    int32_t wait_status;
    osal::OsalReturnType result = osal::OsalReturnType::kFail;

    pid_t terminated_pid = wait(&wait_status);

    if (terminated_pid > 0)
    {
        result = osal::OsalReturnType::kSuccess;
        pid = terminated_pid;
        status = wait_status;
    }
    else
    {
        /// exiting with pid == 0 is perfectly normal behaviour when all process groups are in the Off state.
        LM_LOG_DEBUG() << "wait failed: Unable to wait for any child process to terminate. Error:"
                       << errno_message(errno);
    }

    return result;
}

OsalReturnType ProcessLauncher::waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout)
{
    if (!sync)
    {
        LM_LOG_ERROR() << "Invalid shared memory pointer: The shared memory pointer is null.";
        return OsalReturnType::kFail;
    }

    const auto result = sync->send_sync_.timedWait(timeout);
    if (result != OsalReturnType::kSuccess)
    {
        return result;
    }

    // The semaphore is also posted when the process terminates, so only the reported_running_ flag tells whether the
    // process actually reported kRunning.
    if (!sync->hasReportedRunning())
    {
        return OsalReturnType::kFail;
    }

    return OsalReturnType::kSuccess;
}

}  // namespace score::mw::lifecycle::internal::osal

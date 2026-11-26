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
#include <unistd.h>
#include <signal.h>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/common/signal_safe_log.hpp"
#include "score/mw/launch_manager/osal/security_policy.hpp"
#include "score/mw/launch_manager/osal/set_affinity.hpp"
#include "score/mw/launch_manager/osal/set_groups.hpp"
#include "score/mw/launch_manager/osal/start_process.hpp"
#include "score/mw/launch_manager/osal/sys_exit.hpp"
#include <score/assert.hpp>
#include <cerrno>
#include <cstdlib>

namespace score::mw::lifecycle::internal::osal
{

namespace
{

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

/// @brief Struct to hold configuration parameters for the child process.
struct ChildProcessConfig
{
    /// @brief child process startup configurations
    const configuration::ComponentConfig& config;

    /// @brief fd file descriptor of the shared memory segment.
    int fd;

    /// @brief sync Pointer to the shared memory block
    IpcCommsP shared_block;
};

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
void handleComms(ChildProcessConfig& param)
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
void changeCurrentWorkingDirectory(const configuration::DeploymentConfig& config)
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
void applyResourceLimits(const configuration::Sandbox& config)
{
    for (const auto& rlimit_config : config.rlimits)
    {
        applyLimitOrDie(rlimit_config.resource, rlimit_config.limit, rlimitResourceName(rlimit_config.resource));
    }
}

/// @details The implementation should be async signal safe.
void changeSecurityPolicy(const configuration::Sandbox& config)
{
    if (config.security_policy.has_value() && !config.security_policy.value().empty())
    {
        if (setSecurityPolicy(config.security_policy.value().c_str()) != 0)
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

/// @brief Sets up all the scheduling and security parameters described in the config, for the current process.
/// @details The implementation should be async signal safe.
/// @param config the configuration to use
/// @return kFail if any operation fails, kSuccess otherwise
OsalReturnType setSchedulingAndSecurity(const configuration::Sandbox& config)
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
        if (-1 == setaffinity(config.affinity_mask.value()))
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
    if (-1 == setgroups(supplementary_gids_number, config.supplementary_group_ids.data()))
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

/// @brief Handles the execution of the child process after forking.
/// @details The implementation should be async signal safe.
/// @param[in] param Reference to child process configuration.
[[noreturn]] void handleChildProcess(ChildProcessConfig& param)
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

}  // namespace

OsalReturnType
startProcess(ProcessID& pid, const configuration::ComponentConfig& config, int comms_fd, const IpcCommsP& comms)
{
    OsalReturnType result = OsalReturnType::kFail;

    pid = fork();

    if (pid == kPosixSuccess)
    {
        /*
         * From this point on, only async signal safe functions can be
         * used. `fork` only copies the current thread, so any locks
         * which were held at that time will never be released.
         * See `man 2 fork`.
         */
        ChildProcessConfig param = {config, comms_fd, comms};
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

    return result;
}

}  // namespace score::mw::lifecycle::internal::osal

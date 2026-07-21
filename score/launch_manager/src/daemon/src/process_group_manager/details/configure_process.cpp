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

#ifndef CONFIGURE_PROCESS_HPP_INCLUDED
#define CONFIGURE_PROCESS_HPP_INCLUDED

#include <limits.h>

#include <libgen.h>
#include <sys/resource.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <string_view>

#include "score/launch_manager/src/daemon/src/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/security_policy.hpp"
#include "score/mw/launch_manager/osal/set_affinity.hpp"
#include "score/mw/launch_manager/osal/set_groups.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"

/*
 * In this file only async signal safe functions can be used, as these functions
 * are called between `fork` and `execve`.
 *
 * This is necessary because `fork` only copies the current thread, so any locks
 * which were held at that time will never be released. See `man 2 fork`.
 */

namespace
{

/// @brief Writes a message to stderr in an async signal safe way.
/// @warning This will exit with failure if not successful.
void stderr_print(const std::string_view message)
{
    size_t offset = 0;
    const size_t length = message.length();

    while (offset < length)
    {
        const auto result = write(STDERR_FILENO, &message.at(offset), length - offset);

        if (result == -1)
        {
            if (errno != EINTR)
                _exit(EXIT_FAILURE);
        }
        else
        {
            offset += result;
        }
    }
}

}  // namespace

namespace score
{

namespace lcm
{

namespace internal
{

void safe_log_and_exit(std::initializer_list<std::string_view> parts)
{
    for (auto part : parts)
        stderr_print(part);

    _exit(EXIT_FAILURE);
}

void safe_log_errno_and_exit(std::initializer_list<std::string_view> parts)
{
    for (auto part : parts)
        stderr_print(part);

#if _GNU_SOURCE && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 32
    // strerrordesc_np is documented as async signal safe.
    stderr_print(": ");
    stderr_print(strerrordesc_np(errno));
#elif __QNXNTO__
    // strerror on QNX is documented as async signal safe.
    stderr_print(": ");
    stderr_print(strerror(errno));
#else
    // Otherwise we cannot rely on strerror being safe.
#endif

    _exit(EXIT_FAILURE);
}

void setLimit(const int resource, const std::size_t amount, const std::string_view rlimit_name)
{
    if (amount == 0)
        return;

    const struct rlimit limit{
        .rlim_cur = amount,
        .rlim_max = amount,
    };

    if (setrlimit(resource, &limit) == -1)
        safe_log_errno_and_exit({"Failed to set rlimit ", rlimit_name});
}

void setSchedulingAndSecurity(const osal::OsalConfig& config)
{
    // Set process group id to be equal to the pid
    // setpgid will fail if called by a session leader (which LCMd is), so skip
    if (config.comms_type_ != osal::CommsType::kLaunchManager)
    {
        if (setpgid(0, getpid()) != 0)
            safe_log_errno_and_exit({"Failed to set process group"});
    }

    // Set scheduling policy with sched_setscheduler
    sched_param sch_param{};

    sch_param.sched_priority = config.scheduling_priority_;

    // TODO: Clamping should be done when the config is loaded
    // https://github.com/eclipse-score/lifecycle/issues/304
    if (sch_param.sched_priority < sched_get_priority_min(config.scheduling_policy_))
    {
        sch_param.sched_priority = sched_get_priority_min(config.scheduling_policy_);
    }
    else if (sch_param.sched_priority > sched_get_priority_max(config.scheduling_policy_))
    {
        sch_param.sched_priority = sched_get_priority_max(config.scheduling_policy_);
    }

    if (sched_setscheduler(0, config.scheduling_policy_, &sch_param) == -1)
        safe_log_errno_and_exit({"Failed to set scheduler"});

    if (osal::setaffinity(config.cpu_mask_) == -1)
        safe_log_errno_and_exit({"Failed to set processor affinity"});

    if (setgid(config.gid_) == -1)
        safe_log_errno_and_exit({"Failed to set group"});

    // Note: the type of the first parameter of setgroups() differs in Linux and QNX, so we use osal
    const auto gids_size = config.supplementary_gids_.size();
    if (gids_size > 0)
    {
        const auto gids_data = config.supplementary_gids_.data();
        if (osal::setgroups(gids_size, gids_data) == -1)
            safe_log_errno_and_exit({"Failed to set supplementary groups"});
    }

    if (setuid(config.uid_) == -1)
        safe_log_errno_and_exit({"Failed to set user"});
}

void changeCurrentWorkingDirectory(const osal::OsalConfig& config)
{
    // Notice that this next static variable is duplicated by the fork() and so does not need
    // any protection by a mutex although at first sight you may think it could need one.
    static char path_copy[PATH_MAX + 1U] = {0};

    // TODO: This should be validated when the config is loaded
    // https://github.com/eclipse-score/lifecycle/issues/304
    if (config.executable_path_.size() >= PATH_MAX)
        safe_log_and_exit({"Executable path is too long"});

    if (chdir(dirname(strncpy(path_copy, config.executable_path_.c_str(), PATH_MAX))) == -1)
        safe_log_errno_and_exit({"Failed to change working directory"});
}

void implementMemoryResourceLimits(const osal::OsalConfig& config)
{
    setLimit(RLIMIT_DATA, config.resource_limits_.data_, "RLIMIT_DATA");
    setLimit(RLIMIT_AS, config.resource_limits_.as_, "RLIMIT_AS");
    setLimit(RLIMIT_STACK, config.resource_limits_.stack_, "RLIMIT_STACK");

    // Note about cpu limit:
    // Using setrlimit, this imposes a maximum time that a process will run for, which might not be
    // what you intend? Probably you'll want a maximum time in a time-slice, but you don't get that
    // with limits set by setrlimit...
    setLimit(RLIMIT_CPU, config.resource_limits_.cpu_, "RLIMIT_CPU");
}

void changeSecurityPolicy(const osal::OsalConfig& config)
{
    if (config.security_policy_ == "")
        return;

    if (osal::setSecurityPolicy(config.security_policy_.c_str()) != 0)
        safe_log_errno_and_exit({"Failed to set security policy ", config.security_policy_});
}

}  // namespace internal

}  // namespace lcm

}  // namespace score

#endif

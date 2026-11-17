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

#ifndef OSAL_DETAILS_QNX_START_PROCESS_INTERNAL_HPP_INCLUDED
#define OSAL_DETAILS_QNX_START_PROCESS_INTERNAL_HPP_INCLUDED

#include <sched.h>
#include <secpol/secpol.h>
#include <spawn.h>
#include <sys/resource.h>
#include <signal.h>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/os/errno.h"
#include "score/os/fcntl.h"
#include "score/os/fcntl_impl.h"
#include "score/os/qnx/secpol_impl.h"
#include "score/os/qnx/unistd_impl.h"
#include "score/os/sched_impl.h"
#include "score/os/spawn_impl.h"
#include "score/os/unistd.h"
#include <cerrno>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace score::mw::lifecycle::internal::osal::detail
{

/// @brief Production OS implementations used by startProcessImpl()
struct SpawnOs
{
    score::os::SpawnImpl spawn{};
    score::os::qnx::details::SecpolImpl secpol{};
    score::os::qnx::QnxUnistdImpl qnx_unistd{};
    score::os::FcntlImpl fcntl{};
    score::os::internal::UnistdImpl unistd{};
    score::os::SchedImpl sched{};
};

/// @brief Returns the description of @p error for log messages.
inline std::string_view errorMessage(const score::os::Error& error) noexcept
{
    return errno_message(error.GetOsDependentErrorCode());
}

/// @brief Logs a failed OS call while starting @p executable.
/// @return true if @p result holds a value, false otherwise
template <class T>
bool succeeded(
    const score::cpp::expected<T, score::os::Error>& result,
    std::string_view operation,
    std::string_view executable)
{
    if (result.has_value())
    {
        return true;
    }
    LM_LOG_ERROR() << operation << "failed for" << executable << "with error:" << errorMessage(result.error());
    return false;
}

/// @brief Returns the path of the executable of @p config for log messages.
inline std::string_view executableOf(const configuration::ComponentConfig& config) noexcept
{
    return config.deployment_config.process_command.executablePath();
}

/// @brief Owns the posix_spawn attribute and file action objects and the working directory fd.
/// @tparam OsT See startProcessImpl()
template <class OsT>
class SpawnResources
{
  public:
    explicit SpawnResources(const OsT& os) noexcept : os_(os)
    {
    }

    SpawnResources(const SpawnResources&) = delete;
    SpawnResources& operator=(const SpawnResources&) = delete;
    SpawnResources(SpawnResources&&) = delete;
    SpawnResources& operator=(SpawnResources&&) = delete;

    ~SpawnResources()
    {
        if (cwd_fd_ >= 0)
        {
            static_cast<void>(os_.unistd.close(cwd_fd_));
        }
        if (file_actions_initialized_)
        {
            static_cast<void>(os_.spawn.posix_spawn_file_actions_destroy(&file_actions_));
        }
        if (attr_initialized_)
        {
            static_cast<void>(os_.spawn.posix_spawnattr_destroy(&attr_));
        }
    }

    /// @param executable Path of the executable to start, for log messages
    /// @return true on success, false otherwise (the reason is logged)
    bool init(std::string_view executable)
    {
        if (!succeeded(os_.spawn.posix_spawnattr_init(&attr_), "posix_spawnattr_init", executable))
        {
            return false;
        }
        attr_initialized_ = true;

        if (!succeeded(
                os_.spawn.posix_spawn_file_actions_init(&file_actions_), "posix_spawn_file_actions_init", executable))
        {
            return false;
        }
        file_actions_initialized_ = true;
        return true;
    }

    posix_spawnattr_t* attr()
    {
        return &attr_;
    }

    posix_spawn_file_actions_t* fileActions()
    {
        return &file_actions_;
    }

    void setCwdFd(int fd)
    {
        cwd_fd_ = fd;
    }

  private:
    const OsT& os_;
    posix_spawnattr_t attr_{};
    posix_spawn_file_actions_t file_actions_{};
    bool attr_initialized_{false};
    bool file_actions_initialized_{false};
    int cwd_fd_{-1};
};

/// @brief Returns the configured stack limit if it can be set as maximum stack size via the spawn attributes.
/// @return The soft stack limit, or std::nullopt if none is configured or it is unlimited or too large
inline std::optional<std::uint32_t> spawnStackLimit(const configuration::Sandbox& sandbox) noexcept
{
    for (const auto& rlimit_config : sandbox.rlimits)
    {
        if (rlimit_config.resource == RLIMIT_STACK && rlimit_config.limit.rlim_cur != RLIM_INFINITY &&
            rlimit_config.limit.rlim_cur <= std::numeric_limits<std::uint32_t>::max())
        {
            return static_cast<std::uint32_t>(rlimit_config.limit.rlim_cur);
        }
    }
    return std::nullopt;
}

/// @brief Sets the process group id to be equal to the pid.
template <class OsT>
bool setProcessGroup(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    if (!succeeded(os.spawn.posix_spawnattr_setpgroup(attr, 0), "posix_spawnattr_setpgroup", executableOf(config)))
    {
        return false;
    }
    flags |= POSIX_SPAWN_SETPGROUP;
    return true;
}

/// @brief Resets all signals to their default action and unblocks them, so the new process does not depend on the
/// signal dispositions and the signal mask of the Launch Manager.
/// @details Caught signals are reset by exec anyway, but ignored signals and the signal mask of the calling thread
/// would otherwise be inherited.
template <class OsT>
bool setSignals(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    sigset_t all_signals{};
    static_cast<void>(sigfillset(&all_signals));
    if (!succeeded(
            os.spawn.posix_spawnattr_setsigdefault(attr, &all_signals),
            "posix_spawnattr_setsigdefault",
            executableOf(config)))
    {
        return false;
    }

    sigset_t no_signals{};
    static_cast<void>(sigemptyset(&no_signals));
    if (!succeeded(
            os.spawn.posix_spawnattr_setsigmask(attr, &no_signals), "posix_spawnattr_setsigmask", executableOf(config)))
    {
        return false;
    }
    flags |= POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK;
    return true;
}

/// @brief Sets the CPU affinity, if configured.
template <class OsT>
bool setAffinity(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    const auto& affinity_mask = config.deployment_config.sandbox.affinity_mask;
    if (!affinity_mask.has_value())
    {
        return true;
    }
    if (!succeeded(
            os.spawn.pthread_spawnattr_setrunmask_np(attr, affinity_mask.value()),
            "pthread_spawnattr_setrunmask_np",
            executableOf(config)))
    {
        return false;
    }
    flags |= POSIX_SPAWN_EXPLICIT_CPU;
    return true;
}

/// @brief Sets the user and group id.
template <class OsT>
bool setCredentials(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    const auto& sandbox = config.deployment_config.sandbox;
    const auto cred_result = os.spawn.posix_spawnattr_setcred(attr, sandbox.uid, sandbox.gid);
    if (!cred_result.has_value())
    {
        LM_LOG_ERROR() << "posix_spawnattr_setcred(" << sandbox.uid << "," << sandbox.gid << ") failed for"
                       << executableOf(config) << "with error:" << errorMessage(cred_result.error());
        return false;
    }
    flags |= POSIX_SPAWN_SETCRED;
    return true;
}

/// @brief Sets the maximum stack size, if a stack limit is configured.
/// @details The stack of the main thread already exists while the process is held, so a stack limit applied
/// afterwards with prlimit() would not affect it.
template <class OsT>
bool setStackLimit(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    const auto stack_limit = spawnStackLimit(config.deployment_config.sandbox);
    if (!stack_limit.has_value())
    {
        return true;
    }
    if (!succeeded(
            os.spawn.posix_spawnattr_setstackmax(attr, stack_limit.value()),
            "posix_spawnattr_setstackmax",
            executableOf(config)))
    {
        return false;
    }
    flags |= POSIX_SPAWN_SETSTACKMAX;
    return true;
}

/// @brief Opens the working directory and sets it in the spawn attributes. The fd is owned by @p resources.
template <class OsT>
bool setWorkingDirectory(
    const OsT& os,
    SpawnResources<OsT>& resources,
    const configuration::ComponentConfig& config,
    std::uint32_t& flags)
{
    const auto& working_dir = config.deployment_config.working_dir;

    // working_dir is set by configuration, so it should always be valid.
    const auto cwd_result = os.fcntl.open(
        working_dir.c_str(),
        score::os::Fcntl::Open::kReadOnly | score::os::Fcntl::Open::kDirectory | score::os::Fcntl::Open::kCloseOnExec);
    if (!cwd_result.has_value())
    {
        LM_LOG_ERROR() << "open(" << working_dir << ") failed for" << executableOf(config)
                       << "with error:" << errorMessage(cwd_result.error());
        return false;
    }
    resources.setCwdFd(cwd_result.value());

    const auto setcwd_result = os.spawn.posix_spawnattr_setcwd_np(resources.attr(), cwd_result.value());
    if (!setcwd_result.has_value())
    {
        LM_LOG_ERROR() << "posix_spawnattr_setcwd_np(" << working_dir << ") failed for" << executableOf(config)
                       << "with error:" << errorMessage(setcwd_result.error());
        return false;
    }
    flags |= POSIX_SPAWN_SETCWD;
    return true;
}

/// @brief Sets the configured security policy type, if any. Missing security policy support is ignored.
/// @details The flag POSIX_SPAWN_SETTYPEID is set by the secpol library itself.
template <class OsT>
bool setSecurityPolicy(const OsT& os, posix_spawnattr_t* attr, const configuration::ComponentConfig& config)
{
    const auto& security_policy = config.deployment_config.sandbox.security_policy;
    if (!security_policy.has_value() || security_policy.value().empty())
    {
        return true;
    }

    // The configured name refers directly to a type name
    const auto typeid_result =
        os.secpol.secpol_posix_spawnattr_settypeid(nullptr, attr, security_policy.value().c_str(), SECPOL_TYPE_NAME);
    if (!typeid_result.has_value() && typeid_result.error() != score::os::Error::Code::kOperationNotSupported)
    {
        LM_LOG_ERROR() << "secpol_posix_spawnattr_settypeid(" << security_policy.value() << ") failed for"
                       << executableOf(config) << "with error:" << errorMessage(typeid_result.error());
        return false;
    }
    return true;
}

/// @brief Adds @p flags to the flags of the spawn attributes, keeping the flags set by the attribute functions.
template <class OsT>
bool applyFlags(
    const OsT& os,
    posix_spawnattr_t* attr,
    const configuration::ComponentConfig& config,
    std::uint32_t flags)
{
    std::uint32_t current_flags = 0U;
    return succeeded(
               os.spawn.posix_spawnattr_getxflags(attr, &current_flags),
               "posix_spawnattr_getxflags",
               executableOf(config)) &&
           succeeded(
               os.spawn.posix_spawnattr_setxflags(attr, current_flags | flags),
               "posix_spawnattr_setxflags",
               executableOf(config));
}

/// @brief Duplicates @p comms_fd to IpcCommsSync::sync_fd in the new process, if it communicates.
/// @details The duplicated fd does not have FD_CLOEXEC set, so it remains open in the new process. Note that
/// comms_fd == IpcCommsSync::sync_fd cannot happen as IpcCommsSync::sync_fd is being reserved during startup.
template <class OsT>
bool addCommsFd(
    const OsT& os,
    posix_spawn_file_actions_t* file_actions,
    const configuration::ComponentConfig& config,
    int comms_fd,
    const IpcCommsP& comms)
{
    if (!comms)
    {
        return true;
    }
    return succeeded(
        os.spawn.posix_spawn_file_actions_adddup2(file_actions, comms_fd, IpcCommsSync::sync_fd),
        "posix_spawn_file_actions_adddup2",
        executableOf(config));
}

/// @brief Fills the spawn attributes and file actions from the configuration.
/// @tparam OsT See startProcessImpl()
/// @return true on success, false otherwise (the reason is logged)
template <class OsT>
bool prepareSpawn(
    const OsT& os,
    SpawnResources<OsT>& resources,
    const configuration::ComponentConfig& config,
    int comms_fd,
    const IpcCommsP& comms)
{
    posix_spawnattr_t* attr = resources.attr();

    // Start the process held, so the scheduling, supplementary groups and resource limits can be applied before it
    // runs (see configureHeldProcess())
    std::uint32_t flags = POSIX_SPAWN_HOLD;

    const bool attributes_set = setProcessGroup(os, attr, config, flags) && setSignals(os, attr, config, flags) &&
                                setAffinity(os, attr, config, flags) && setCredentials(os, attr, config, flags) &&
                                setStackLimit(os, attr, config, flags) &&
                                setWorkingDirectory(os, resources, config, flags) &&
                                setSecurityPolicy(os, attr, config);
    if (!attributes_set)
    {
        return false;
    }

    // Must come after all other attribute functions, so the flags set by them (e.g. POSIX_SPAWN_SETTYPEID) are kept
    if (!applyFlags(os, attr, config, flags))
    {
        return false;
    }

    return addCommsFd(os, resources.fileActions(), config, comms_fd, comms);
}

/// @brief Sets the scheduling policy and priority of the held process @p pid.
/// @details The scheduling is not set via the spawn attributes, as posix_spawn() blocks until the main thread of the
/// new process has completed its initialization, which it performs at the scheduling priority of the new process.
/// A priority lower than the one of the Launch Manager would cause a priority inversion. Without the spawn
/// attributes, the new process inherits the scheduling of the calling thread for its initialization instead.
/// @tparam OsT See startProcessImpl()
/// @return true on success, false otherwise (the reason is logged)
template <class OsT>
bool setScheduling(const OsT& os, ProcessID pid, const configuration::ComponentConfig& config)
{
    const auto& sandbox = config.deployment_config.sandbox;

    // The priority was clamped to the valid range of the policy when the configuration was loaded
    sched_param sch_param{};
    sch_param.sched_priority = sandbox.scheduling_priority;
    const auto sched_result = os.sched.sched_setscheduler(pid, sandbox.scheduling_policy, &sch_param);
    if (!sched_result.has_value())
    {
        LM_LOG_ERROR() << "sched_setscheduler(" << sandbox.scheduling_policy << "," << sandbox.scheduling_priority
                       << ") failed for" << executableOf(config) << "( pid" << pid
                       << ") with error:" << errorMessage(sched_result.error());
        return false;
    }
    return true;
}

// req-Id: comp_req__launch_man__process_rlimit_support
/// @brief Applies the scheduling, supplementary groups and resource limits to the held process @p pid.
/// @tparam OsT See startProcessImpl()
/// @return true on success, false otherwise (the reason is logged)
template <class OsT>
bool configureHeldProcess(const OsT& os, ProcessID pid, const configuration::ComponentConfig& config)
{
    const auto& sandbox = config.deployment_config.sandbox;

    // The scheduling is explicitly set after the process is spawned, to avoid priority inversion during initialization.
    // Priority inversion could happen if the posix_spawn runs with lower priority and the higher priority Launch
    // Manager thread is waiting for it.
    if (!setScheduling(os, pid, config))
    {
        return false;
    }

    // Always set the supplementary groups, so the groups of the Launch Manager are not inherited
    const auto supplementary_gids_number = static_cast<std::int32_t>(sandbox.supplementary_group_ids.size());
    const auto groups_result = os.qnx_unistd.setgroupspid(
        supplementary_gids_number,
        supplementary_gids_number != 0 ? sandbox.supplementary_group_ids.data() : nullptr,
        pid);
    if (!groups_result.has_value())
    {
        LM_LOG_ERROR() << "setgroupspid() failed for" << executableOf(config) << "( pid" << pid
                       << ") with error:" << errorMessage(groups_result.error());
        return false;
    }

    // baselibs provides no abstraction for prlimit()
    for (const auto& rlimit_config : sandbox.rlimits)
    {
        if (-1 == ::prlimit(pid, rlimit_config.resource, &rlimit_config.limit, nullptr))
        {
            // Saved before logging, which may change errno
            const int error_number = errno;
            LM_LOG_ERROR() << "prlimit(" << rlimit_config.resource << ") failed for" << executableOf(config) << "( pid"
                           << pid << ") with error:" << errno_message(error_number);
            return false;
        }
    }

    return true;
}

/// @brief Implementation of osal::startProcess() using posix_spawn() and its QNX extensions.
/// @details The process is spawned held, so the scheduling, supplementary groups and resource limits can be applied
/// before it runs. See osal::startProcess() for the parameters and the return value.
/// @tparam OsT Provides the OS functions via the members spawn (score::os::Spawn), secpol (score::os::qnx::Secpol),
/// qnx_unistd (score::os::qnx::QnxUnistd), fcntl (score::os::Fcntl), unistd (score::os::Unistd) and sched
/// (score::os::Sched).
template <class OsT = SpawnOs>
OsalReturnType startProcessImpl(
    const OsT& os,
    ProcessID& pid,
    const configuration::ComponentConfig& config,
    int comms_fd,
    const IpcCommsP& comms)
{
    pid = -1;

    SpawnResources<OsT> resources{os};
    if (!resources.init(executableOf(config)))
    {
        return OsalReturnType::kFail;
    }

    if (!prepareSpawn(os, resources, config, comms_fd, comms))
    {
        return OsalReturnType::kFail;
    }

    // argv and envp are assembled when the configuration is loaded and are null-terminated
    char* const* argv = config.deployment_config.process_command.argv();
    char* const* envp = config.deployment_config.environmental_variables.envp();

    ProcessID child_pid = -1;
    if (!succeeded(
            os.spawn.posix_spawn(&child_pid, argv[0], resources.fileActions(), resources.attr(), argv, envp),
            "posix_spawn",
            executableOf(config)))
    {
        return OsalReturnType::kFail;
    }
    pid = child_pid;

    // The process is held, so storing the pid cannot race with the check at the client end
    if (comms)
    {
        comms->pid_ = pid;
    }

    if (!configureHeldProcess(os, pid, config))
    {
        // The process exists, so report it as started and let it terminate. This way the termination is reported
        // via the normal path, like a forked process that fails to apply its configuration on Linux.
        // baselibs provides no abstraction for kill()
        if (-1 == ::kill(pid, SIGKILL))
        {
            // Saved before logging, which may change errno
            const int error_number = errno;
            LM_LOG_ERROR() << "Unable to kill held process" << executableOf(config) << "( pid" << pid
                           << ") with error:" << errno_message(error_number);
        }
        return OsalReturnType::kSuccess;
    }

    if (-1 == ::kill(pid, SIGCONT))
    {
        // Saved before logging, which may change errno
        const int error_number = errno;
        LM_LOG_ERROR() << "Unable to continue held process" << executableOf(config) << "( pid" << pid
                       << ") with error:" << errno_message(error_number);
        static_cast<void>(::kill(pid, SIGKILL));
    }

    return OsalReturnType::kSuccess;
}

}  // namespace score::mw::lifecycle::internal::osal::detail

#endif  // OSAL_DETAILS_QNX_START_PROCESS_INTERNAL_HPP_INCLUDED

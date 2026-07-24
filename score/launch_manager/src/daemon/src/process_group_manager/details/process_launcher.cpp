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

#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "score/launch_manager/src/daemon/src/process_group_manager/details/configure_process.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/control/control_client_channel.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/security_policy.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

namespace
{

using score::lcm::internal::safe_log_and_exit;
using score::lcm::internal::safe_log_errno_and_exit;
using score::lcm::internal::osal::CommsType;
using score::lcm::internal::osal::IpcCommsSync;
using score::lcm::internal::osal::OsalConfig;

/// @warning This will exit with failure if not successful.
void handleComms(score::lcm::internal::osal::ChildProcessConfig& param)
{
    // kNoComms !fd3 & !fd4
    // kReporting  fd3 & !fd4
    // kControlClient  fd3 & fd4
    // kLaunchManager  does not matter
    if (!param.shared_block)
    {
        // kNoComms, fds are CLOEXEC
        return;
    }

    param.fd = dup2(param.fd, param.shared_block->sync_fd);  // always make sure we are using fd=3
    param.shared_block->pid_ = getpid();                     // Store pid for check at client end

    // It must be ensured that sync_fd (f3) and control_client_handler_nudge_fd (fd4) remain open depending on
    // the communication type. Flag FD_CLOEXEC is cleared conditionally to ensure that the
    // respective file descriptor remains open after the execve call.
    switch (param.shared_block->comms_type_)
    {
        case CommsType::kNoComms:
            // in the current implementation this case means param.shared_block == nullptr and is handled above
            break;
        case CommsType::kReporting:
            if (fcntl(IpcCommsSync::sync_fd, F_SETFD, 0) == -1)
                safe_log_and_exit({"Failed to set sync_fd flags"});

            close(IpcCommsSync::control_client_handler_nudge_fd);

            break;
        case CommsType::kControlClient:
            if (fcntl(IpcCommsSync::sync_fd, F_SETFD, 0) == -1)
                safe_log_and_exit({"Failed to set sync_fd flags"});

            if (fcntl(IpcCommsSync::control_client_handler_nudge_fd, F_SETFD, 0) == -1)
                safe_log_errno_and_exit({"Failed to set control_client_handler_nudge_fd flags"});

            break;
        case CommsType::kLaunchManager:
            // nothing to do here
            break;
        default:
            safe_log_and_exit({"Unknown CommsType"});
            break;
    }
}

// TODO: Move this into the config loader
// https://github.com/eclipse-score/lifecycle/issues/304
bool validateConfig(const OsalConfig* config)
{
    if (config->scheduling_priority_ < sched_get_priority_min(config->scheduling_policy_))
    {
        LM_LOG_WARN() << "Scheduling priority" << config->scheduling_priority_ << "is below minimum for policy"
                      << config->scheduling_policy_;
    }
    else if (config->scheduling_priority_ > sched_get_priority_max(config->scheduling_policy_))
    {
        LM_LOG_WARN() << "Scheduling priority" << config->scheduling_priority_ << "is above maximum for policy"
                      << config->scheduling_policy_;
    }

    return true;
}

}  // namespace

namespace score
{

namespace lcm
{

namespace internal
{

namespace osal
{

OsalReturnType IProcess::startProcess(ProcessID* pid, IpcCommsP* block, const OsalConfig* config)
{
    OsalReturnType result = OsalReturnType::kFail;

    if ((pid && block && config && config->executable_path_ != "" && config->argv_[0U]))
    {
        if (!validateConfig(config))
            return result;

        int fd = -1;
        *pid = -1;
        *block = nullptr;
        bool comms_result = true;

        if (config->comms_type_ != CommsType::kNoComms)
        {
            comms_result = setupComms(*block, fd, *config);
        }

        if (comms_result)
        {
            /// @todo need to recheck after logging framework implementation.
            static_cast<void>(fflush(stdout));

            *pid = fork();

            if (*pid == kPosixSuccess)
            {
                /*
                 * From this point on only async signal safe functions can be
                 * used.
                 *
                 * This is necessary because `fork` only copies the current
                 * thread, so any locks which were held at that time will never
                 * be released. See `man 2 fork`.
                 */

                ChildProcessConfig param = {config, fd, *block};
                handleChildProcess(param);
                result = OsalReturnType::kSuccess;
            }
            else if (*pid > kPidZero)
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
            LM_LOG_ERROR()
                << "Shared memory creation failed: Unable to create shared memory for kRunning communication.";
        }

        if (fd >= 0)
        {
            close(fd);
        }
    }
    else
    {
        LM_LOG_ERROR()
            << "Invalid input parameters: Ensure process_id, config, executable_path, and argv are correctly provided.";

        return result;
    }

    return result;
}

inline bool IProcess::setupComms(IpcCommsP& block, int& fd, const OsalConfig& config)
{
    bool comms_result = true;
    char shm_name[static_cast<uint32_t>(score::lcm::internal::ProcessLimits::maxLocalBuffSize)];
    size_t length = sizeof(IpcCommsSync);

    if (CommsType::kControlClient == config.comms_type_)
    {
        length += sizeof(ControlClientChannel);
    }

    static_cast<void>(snprintf(shm_name,
        static_cast<uint32_t>(score::lcm::internal::ProcessLimits::maxLocalBuffSize),
        "/ipc_shared_mem%u",
        shm_name_counter++));

    fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0U);

    if (fd < 0)
    {
        LM_LOG_ERROR() << "shm_open failed:" << config.executable_path_
                       << "Unable to open shared memory object. Error:" << score::lcm::internal::errno_message(errno);
        comms_result = false;
    }
    else
    {
        shm_unlink(shm_name);

        if (ftruncate(fd, static_cast<int>(length)))  // failure -1
        {
            comms_result = false;
            LM_LOG_ERROR() << "ftruncate failed:" << config.executable_path_
                           << "Unable to set size of shared memory file descriptor. Error:"
                           << score::lcm::internal::errno_message(errno);
        }

        if (config.comms_type_ == CommsType::kControlClient)
        {
            block = initializeControlClient(fd, config);
        }
        else
        {
            block = IpcCommsSync::getCommsObject(fd);
        }
        if (block)
        {
            block->comms_type_ = config.comms_type_;
            if (!initializeSemaphores(block))
            {
                LM_LOG_ERROR() << "Semaphore init failed:" << config.short_name_
                               << "Unable to initialize send_sync or reply_sync semaphore.";
                comms_result = false;
            }
        }
        else
        {
            comms_result = false;
        }
    }

    return comms_result;
}

inline IpcCommsP IProcess::initializeControlClient(int& fd, const OsalConfig& config)
{
    LM_LOG_DEBUG() << "Initialize the control client for" << config.short_name_ << " process";
    /* Initialise the control client communications */
    IpcCommsP shared_block = nullptr;
    ControlClientChannelP scc = ControlClientChannel::initializeControlClientChannel(fd, &shared_block);
    if (!scc)
    {
        LM_LOG_ERROR() << "Failed to obtain ControlClientChannel for " << config.short_name_
                       << ": initializeControlClientChannel returned nullptr";
        return nullptr;  // Caller will see shared_block maybe null and treat as failure later.
    }
    scc->initialize();
    return shared_block;
}

inline bool IProcess::initializeSemaphores(IpcCommsP shared_block)
{
    bool result = true;

    if (osal::OsalReturnType::kFail == shared_block->send_sync_.init(0U, true) ||
        osal::OsalReturnType::kFail == shared_block->reply_sync_.init(0U, true))
    {
        result = false;
        LM_LOG_ERROR() << "Semaphore init failed: Unable to initialize send_sync or reply_sync semaphore.";
    }

    return result;
}

/// @warning This will exit with failure if not successful.
inline void IProcess::handleChildProcess(ChildProcessConfig& param)
{
    handleComms(param);

    setSchedulingAndSecurity(*param.config);
    changeCurrentWorkingDirectory(*param.config);
    implementMemoryResourceLimits(*param.config);
    changeSecurityPolicy(*param.config);

    // Finally, execute the process, passing all the arguments and environment variables
    if (execve(param.config->argv_[0], const_cast<char* const*>(param.config->argv_.data()), param.config->envp_) == -1)
        safe_log_errno_and_exit({"Failed to execute process"});
}

OsalReturnType IProcess::requestTermination(ProcessID pid)
{
    LM_LOG_DEBUG() << "Request termination received for" << pid;

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
                           << ". Error:" << score::lcm::internal::errno_message(errno);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType IProcess::forceTermination(ProcessID pid)
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

OsalReturnType IProcess::waitForTermination(osal::ProcessID& pid, int32_t& status)
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
                       << score::lcm::internal::errno_message(errno);
    }

    return result;
}

OsalReturnType IProcess::waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout)
{
    OsalReturnType result = OsalReturnType::kSuccess;

    if (sync)
    {
        if ((sync->send_sync_.timedWait(timeout) == OsalReturnType::kFail) ||
            (sync->reply_sync_.post() == OsalReturnType::kFail))
        {
            LM_LOG_ERROR() << "Semaphore timedWait or post failed: Unable to wait or post on semaphores within the "
                              "specified timeout.";
            result = OsalReturnType::kFail;
        }
        else
        {
            result = sync->send_sync_.timedWait(std::chrono::milliseconds(100));
        }

        // We are not interested in the result of msync, just whether it worked or not.
        // If it did not work, the child process has probably crashed and corrupted the shared memory
        // so we should not try to deinitialize the semaphores.
        // mincore would be more appropriate here, but is not available on QNX
        if (msync(sync.get(), sizeof(IpcCommsSync), MS_ASYNC) == 0)
        {
            if (sync->send_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize send_sync semaphore.";
            }
            if (sync->reply_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize reply_sync semaphore.";
            }
        }
        else
        {
            LM_LOG_WARN() << "Skipping semaphore deinitialization - shared memory region appears invalid: "
                          << score::lcm::internal::errno_message(errno);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid shared memory pointer: The shared memory pointer is null.";
        result = OsalReturnType::kFail;
    }

    return result;
}

}  // namespace osal

}  // namespace internal

}  // namespace lcm

}  // namespace score

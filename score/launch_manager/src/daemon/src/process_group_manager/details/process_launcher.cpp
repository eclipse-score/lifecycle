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
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/common/signal_safe_log.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/shm_open_anon.hpp"
#include "score/mw/launch_manager/osal/start_process.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <score/assert.hpp>
#include <cerrno>
#include <chrono>
#include <cstdio>

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

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
            LM_LOG_ERROR() << "Failed to flush stdout before starting the process.";
        }

        result = osal::startProcess(pid, config, fd, block);
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

OsalReturnType ProcessLauncher::requestTermination(ProcessID pid)
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
        LM_LOG_ERROR() << "Semaphore timedWait failed: Did not receive kRunning within the specified timeout.";
        return result;
    }

    // The semaphore is also posted when the process terminates, so only the reported_running_ flag tells whether the
    // process actually reported kRunning.
    if (!sync->hasReportedRunning())
    {
        LM_LOG_ERROR() << "Process terminated without reporting kRunning.";
        return OsalReturnType::kFail;
    }

    return OsalReturnType::kSuccess;
}

}  // namespace score::mw::lifecycle::internal::osal

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

#include <unistd.h>
#include <cstring>
#include <iostream>

#include "score/mw/launch_manager/common/log.hpp"

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/alive_monitor/details/watchdog/WatchdogImpl.hpp"
#include "score/mw/launch_manager/process_group_manager/alive_monitor_thread.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"
#include "score/mw/launch_manager/process_state_client/process_state_notifier.hpp"
#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"

using namespace std;
using namespace score::lcm::internal;

/// @brief Initializes the LCM daemon.
/// This function initializes the LCM daemon by calling the initialize() method
/// of the provided ProcessGroupManager object. It logs an information message
/// if initialization is successful and a fatal error message if it fails.
/// @param process_group_manager The ProcessGroupManager object to initialize.
/// @return True if initialization succeeds, false otherwise.
bool initializeLCMDaemon(ProcessGroupManager& process_group_manager)
{
    if (process_group_manager.initialize())
    {
        LM_LOG_INFO() << "LCM started successfully";
        return true;
    }
    else
    {
        LM_LOG_FATAL() << "LCM startup failed";
        return false;
    }
}

/// @brief Runs the LCM daemon.
/// This function runs the LCM daemon by calling the run() method of the provided
/// ProcessGroupManager object. It logs an information message if the run is successful,
/// and an error message if it fails.
/// @param process_group_manager The ProcessGroupManager object to run.
/// @return True if the run succeeds, false otherwise.
bool runLCMDaemon(ProcessGroupManager& process_group_manager)
{
    if (process_group_manager.run())
    {
        LM_LOG_DEBUG() << "LCM run successfully";
        return true;
    }
    else
    {
        LM_LOG_ERROR() << "LCM run failed";
        return false;
    }
}

/// @brief Reserves a file descriptor.
/// @param fd file descriptor to reserve.
/// @warning This function can abort if system calls fail.
void reserveFD(int fd)
{
    int open_fd = fcntl(fd, F_GETFD);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        open_fd != -1, "Failed to reserver required file descriptor, file descriptor already in use.");

    int tmp_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        tmp_fd < 0, "Failed to reserver required file descriptor, failed to open temporary file.");

    if (fd != tmp_fd)
    {
        if (dup2(tmp_fd, fd) == -1)
        {
            close(tmp_fd);
            std::cerr << "Failed to reserver required file descriptor, couldn't duplicate fd with required number"
                      << std::strerror(errno);
            std::abort();
        }

        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        {
            close(tmp_fd);
            close(fd);
            std::cerr << "Failed to reserver required file descriptor, couldn't set flags on reserved file decriptor"
                      << std::strerror(errno);
            std::abort();
        }

        close(tmp_fd);
    }
}

/// @brief Main function to start the LCM daemon.
/// This function initializes and runs the LCM daemon by creating a ProcessGroupManager,
/// initializing it, and then running it. It returns the appropriate exit code based on
/// the success or failure of the LCM daemon operation.
/// @param argc Number of command-line arguments.
/// @param argv Array of command-line arguments.
/// @return The exit code. 0 for success, non-zero for failure.
// coverity[autosar_cpp14_a15_3_3_violation:FALSE] Only logging occurs outside the try-catch enclosing main().
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    // reserve files descriptor osal::IpcCommsSync::sync_fd (fd3) and
    // osal::IpcCommsSync::control_client_handler_nudge_fd (fd4) for communication tpyes: kNoComms !fd3 & !fd4
    // kReporting  fd3 & !fd4
    // kControlClient  fd3 & fd4
    // kLaunchManager  does not matter
    // the file descriptors are closed inside the handleComms function.
    reserveFD(osal::IpcCommsSync::sync_fd);
    reserveFD(osal::IpcCommsSync::control_client_handler_nudge_fd);

    int exit_code = EXIT_FAILURE;

    try
    {
        /// @todo Check that we're not already running

        // if (-1 == daemon(-1, -1)) {
        //     LM_LOG_FATAL() << "LCM could not daemonize!, error:" << strerror(errno);
        //     return EXIT_FAILURE;
        // }

        LM_LOG_DEBUG() << "Launch Manager Started !!!!";
        std::shared_ptr<score::lcm::IRecoveryClient> recoveryClient{std::make_shared<score::lcm::RecoveryClient>()};
        std::unique_ptr<score::lcm::saf::watchdog::IWatchdogIf> watchdog{
            std::make_unique<score::lcm::saf::watchdog::WatchdogImpl>()};
        auto process_state_notifier = std::make_unique<score::lcm::internal::ProcessStateNotifier>();
        std::unique_ptr<score::lcm::saf::daemon::IAliveMonitor> healthMonitor{
            std::make_unique<score::lcm::saf::daemon::AliveMonitorImpl>(
                recoveryClient, std::move(watchdog), process_state_notifier->constructReceiver())};
        std::unique_ptr<score::lcm::internal::IAliveMonitorThread> aliveMonitorThread{
            std::make_unique<score::lcm::internal::AliveMonitorThread>(std::move(healthMonitor))};

        std::unique_ptr<ProcessGroupManager> process_group_manager = std::make_unique<ProcessGroupManager>(
            std::move(aliveMonitorThread), recoveryClient, std::move(process_state_notifier));

        if (initializeLCMDaemon(*process_group_manager))
        {
            if (runLCMDaemon(*process_group_manager))
            {
                exit_code = EXIT_SUCCESS;
            }
        }

        if (process_group_manager)
        {
            process_group_manager->deinitialize();
            process_group_manager.reset();
        }
    }
    catch (...)
    {
        exit_code = EXIT_FAILURE;
    }

    close(osal::IpcCommsSync::sync_fd);
    close(osal::IpcCommsSync::control_client_handler_nudge_fd);

    LM_LOG_INFO() << "Launch Manager completed with exit code value:" << exit_code;

    return exit_code;
}

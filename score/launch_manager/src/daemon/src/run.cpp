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

#include "score/mw/launch_manager/run.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/configuration/flatbuffer_config_loader.hpp"
#include "score/mw/launch_manager/control/control_provider.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"
#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"
#include "score/mw/launch_manager/watchdog/watchdog_factory.hpp"

namespace score::mw::lifecycle::internal
{

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
    errno = 0;
    const int open_fd_flags = ::fcntl(fd, F_GETFD);
    const bool fd_already_opened = open_fd_flags == 0 && errno == EBADFD;

    if (fd_already_opened)
    {
        std::cerr << "Failed to reserve required file descriptor (" << fd << "), file descriptor already in use. "
                  << std::strerror(errno);
        std::abort();
    }

    int tmp_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (tmp_fd < 0)
    {
        std::cerr << "Failed to reserve required file descriptor (" << fd << "), failed to open temporary file. "
                  << std::strerror(errno);
        std::abort();
    }

    if (fd != tmp_fd)
    {
        if (::dup2(tmp_fd, fd) == -1)
        {
            ::close(tmp_fd);

            std::cerr << "Failed to reserve required file descriptor (" << fd
                      << "), couldn't duplicate fd with required number. " << std::strerror(errno);
            std::abort();
        }

        if (::fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        {
            ::close(tmp_fd);
            ::close(fd);

            std::cerr << "Failed to reserve required file descriptor (" << fd
                      << ") , couldn't set flags on reserved file decriptor. " << std::strerror(errno);
            std::abort();
        }

        ::close(tmp_fd);
    }
}

/// @brief Main function to start the LCM daemon.
/// This function initializes and runs the LCM daemon by creating a ProcessGroupManager,
/// initializing it, and then running it. It returns the appropriate exit code based on
/// the success or failure of the LCM daemon operation.
/// @param argc Number of command-line arguments.
/// @param argv Array of command-line arguments.
/// @return The exit code. 0 for success, non-zero for failure.
int run(int argc, const char* argv[])
{
    constexpr const char* kUsage = "Usage: launch_manager [-c <config>] [-h]\n";
    const char* config_path = "etc/launch_manager_config.bin";
    // Only -c and -h are handled here. All other arguments are ignored, they may belong to the watchdog
    // implementation, which receives the whole command line (see watchdog_factory.hpp). getopt() is not used,
    // because it would read unknown options like -w/dev/watchdog as a cluster of flags including -c.
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};
        if (arg == "-h")
        {
            std::cout << kUsage << "\n"
                      << "Options:\n"
                      << "  -c <config>  Path to the flatbuffer config binary.\n"
                      << "               Default: etc/launch_manager_config.bin\n"
                      << "  -h           Print this help and exit.\n";
            return EXIT_SUCCESS;
        }
        if (arg == "-c")
        {
            if (i + 1 >= argc)
            {
                std::cerr << kUsage;
                return EXIT_FAILURE;
            }
            config_path = argv[++i];
        }
        else if (arg.substr(0, 2) == "-c")
        {
            config_path = argv[i] + 2;
        }
    }

    // reserve files descriptor osal::IpcCommsSync::sync_fd (fd3)
    // the file descriptors are closed inside the handleComms function.
    reserveFD(osal::IpcCommsSync::sync_fd);

    int exit_code = EXIT_FAILURE;

    try
    {
        configuration::FlatbufferConfigLoader config_loader;
        auto config_result = config_loader.load(config_path);
        if (!config_result.has_value())
        {
            LM_LOG_FATAL() << "Failed to load config from:" << std::string_view(config_path);
            return EXIT_FAILURE;
        }
        LM_LOG_DEBUG() << "Launch Manager Started !!!!";

        std::shared_ptr<IRecoveryClient> recoveryClient{std::make_shared<RecoveryClient>()};

        const std::size_t supervised_components = static_cast<std::size_t>(std::count_if(
            config_result.value().graph().components_.begin(),
            config_result.value().graph().components_.end(),
            [](const configuration::ComponentConfig& component) {
                return component.component_properties.application_profile.alive_supervision.has_value();
            }));

        std::unique_ptr<saf::daemon::IAliveMonitor> healthMonitor{std::make_unique<saf::daemon::AliveMonitorImpl>(
            recoveryClient, config_result.value().takeAliveSupervision(), supervised_components)};

        auto watchdog = watchdog::createWatchdog(argc, argv);
        if (!watchdog)
        {
            LM_LOG_FATAL() << "Failed to create watchdog";
            return EXIT_FAILURE;
        }
        auto process_group_manager = std::make_unique<ProcessGroupManager>(
            config_result.value().takeGraph(), std::move(healthMonitor), recoveryClient, std::move(watchdog));

        std::unique_ptr<ControlProvider> control_provider;

        if (process_group_manager->initialize())
        {
            score::Result<std::unique_ptr<ControlProvider>> control_provider_result =
                ControlProvider::Create(process_group_manager.get());

            if (!control_provider_result.has_value())
            {
                LM_LOG_FATAL() << "Failed to set up LmControl service provider:"
                               << control_provider_result.error().Message();
                exit_code = EXIT_FAILURE;
            }
            else
            {
                control_provider = std::move(control_provider_result).value();
                if (runLCMDaemon(*process_group_manager))
                {
                    exit_code = EXIT_SUCCESS;
                }
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

    LM_LOG_INFO() << "Launch Manager completed with exit code value:" << exit_code;

    return exit_code;
}

}  // namespace score::mw::lifecycle::internal

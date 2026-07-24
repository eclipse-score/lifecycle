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

#include <unistd.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <optional>
#include <thread>

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

#include <score/mw/lifecycle/alive.h>
#include <score/mw/lifecycle/report_running.h>

/// @brief CLI configuration options for the cpp_supervised_app_simple process
struct Config
{
    std::uint32_t sleepInMs{50};
};

std::optional<Config> parseOptions(int argc, char* const* argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, "s:h")) != -1)
    {
        switch (static_cast<char>(c))
        {
            case 's':
                config.sleepInMs = std::stoi(optarg);
                break;

            case 'h':
                std::cout << "Usage: \n\
                            -s <Sleep time in ms between alive notifications>\n";
                return std::nullopt;

            default:
                break;
        }
    }
    return config;
}

std::atomic<bool> exitRequested{false};
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        exitRequested = true;
    }
}

void set_process_name()
{
    const char* identifier = getenv("PROCESSIDENTIFIER");
    if (identifier != nullptr)
    {
#if defined(__QNXNTO__)
        if (pthread_setname_np(pthread_self(), identifier) != 0)
        {
            std::cerr << "Failed to set QNX thread name" << std::endl;
        }
#elif defined(__linux__)
        if (prctl(PR_SET_NAME, identifier) < 0)
        {
            std::cerr << "Failed to set process name to " << identifier << std::endl;
        }
#endif
    }
}

/// @brief Minimal example of the Alive API: report Running once, then report
/// alive notifications periodically until the Launch Manager requests shutdown.
/// Unlike cpp_supervised_app, this example has no deadline/checkpoint
/// supervision -- it only demonstrates the plain alive heartbeat.
int main(int argc, char** argv)
{
    set_process_name();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    const auto config = parseOptions(argc, argv);
    if (!config)
    {
        return EXIT_FAILURE;
    }

    std::optional<score::mw::lifecycle::Alive> alive;
    try
    {
        alive.emplace("cpp_supervised_app_simple");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to create Alive instance: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    score::mw::lifecycle::report_running();

    while (!exitRequested)
    {
        alive->ReportAlive();
        std::this_thread::sleep_for(std::chrono::milliseconds(config->sleepInMs));
    }

    return EXIT_SUCCESS;
}

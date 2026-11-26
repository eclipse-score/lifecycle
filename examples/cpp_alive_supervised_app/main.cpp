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
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <thread>

#include <score/mw/lifecycle/alive.h>
#include <score/mw/lifecycle/report_running.h>

namespace
{

/// @brief CLI configuration options for this example application
struct Config
{
    std::uint32_t reportIntervalInMs{50};
};

std::optional<Config> parseOptions(int argc, char* const* argv) noexcept
{
    Config config{};
    int option;
    while ((option = getopt(argc, argv, "s:h")) != -1)
    {
        switch (static_cast<char>(option))
        {
            case 's':
            {
                char* end = nullptr;
                const unsigned long interval = std::strtoul(optarg, &end, 10);
                if ((end == optarg) || (*end != '\0') || (interval == 0UL) || (interval > UINT32_MAX))
                {
                    std::cerr << "Invalid alive notification interval: " << optarg << std::endl;
                    return std::nullopt;
                }
                config.reportIntervalInMs = static_cast<std::uint32_t>(interval);
                break;
            }

            case 'h':
                std::cout << "Usage: \n\
                            -s <Interval in milliseconds between alive notifications> \n";
                return std::nullopt;

            default:
                return std::nullopt;
        }
    }
    return config;
}

std::atomic<bool> exitRequested{false};

void signalHandler(int signal)
{
    if ((signal == SIGINT) || (signal == SIGTERM))
    {
        exitRequested = true;
    }
}

}  // namespace

int main(int argc, char** argv)
{
    // The Launch Manager requests shutdown by sending SIGTERM. When report_running() is
    // used directly, the process must handle that signal itself.
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    const auto config = parseOptions(argc, argv);
    if (!config.has_value())
    {
        return EXIT_FAILURE;
    }

    try
    {
        // Connect to the alive supervision channel provided by the Launch Manager.
        score::mw::lifecycle::Alive alive{"instance"};

        // Report the Running state, so that the component reaches its Ready State.
        score::mw::lifecycle::report_running();

        while (!exitRequested)
        {
            alive.ReportAlive();
            std::this_thread::sleep_for(std::chrono::milliseconds(config->reportIntervalInMs));
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to set up alive supervision: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

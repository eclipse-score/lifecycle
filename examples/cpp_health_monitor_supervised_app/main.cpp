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
#include <atomic>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <iostream>

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

#include <score/health_monitor/deadline.h>
#include <score/health_monitor/deadline_monitor.h>
#include <score/health_monitor/deadline_monitor_builder.h>
#include <score/health_monitor/health_monitor.h>
#include <score/health_monitor/health_monitor_builder.h>
#include <score/health_monitor/tag.h>
#include <score/health_monitor/time_range.h>
#include <score/mw/lifecycle/report_running.h>
#include <thread>

/// @brief CLI configuration options for the demo_application process
struct Config
{
    std::uint32_t delayInMs{50};
    std::uint32_t deadlineMaxInMs{150};
};

std::optional<Config> parseOptions(int argc, char* const* argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, "d:m:s:h")) != -1)
    {
        switch (static_cast<char>(c))
        {
            case 'd':
                // std::cout << "Delay between reporting all checkpoints is: " << optarg << "ms" << std::endl;
                config.delayInMs = static_cast<std::uint32_t>(std::stoi(optarg));
                break;

            case 'm':
                config.deadlineMaxInMs = static_cast<std::uint32_t>(std::stoi(optarg));
                break;

            case 'h':
                std::cout << "Usage: \n\
                            -d <The app measures a deadline between 50ms and the maximum set with -m. You configure the delay inside this deadline measurement> \n\
                            -m <Upper bound of the deadline in ms, default 150. Raise it on targets with unpredictable CPU availability> \n";
                return std::nullopt;

            default:
                break;
        }
    }
    if (config.deadlineMaxInMs < 50U)
    {
        std::cerr << "-m must be at least 50 (the lower bound of the deadline)" << std::endl;
        return std::nullopt;
    }
    return config;
}

std::atomic<bool> exitRequested{false};
std::atomic<bool> stopReportingCheckpoints{false};
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        exitRequested = true;
    }
    else if (signal == SIGUSR1)
    {
        // std::cout << "Received SIGUSR1 signal." << std::endl;
        stopReportingCheckpoints = true;
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

int main(int argc, char** argv)
{
    set_process_name();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);

    const auto config = parseOptions(argc, argv);
    if (!config)
    {
        return EXIT_FAILURE;
    }

    using namespace score::health_monitor;

    auto builder_mon =
        DeadlineMonitorBuilder{}
            .AddDeadline(
                DeadlineTag{"deadline_1"},
                TimeRange{std::chrono::milliseconds(50), std::chrono::milliseconds(config->deadlineMaxInMs)})
            .AddDeadline(
                DeadlineTag{"deadline_2"},
                TimeRange{
                    std::chrono::milliseconds(2),
                    std::chrono::milliseconds(20)});  // Not used, only shows
                                                      // that multiple deadlines can be added

    constexpr MonitorTag ident{"monitor"};

    {
        auto hm_res = HealthMonitorBuilder{}
                          .AddDeadlineMonitor(ident, std::move(builder_mon))
                          .WithInternalProcessingCycle(std::chrono::milliseconds(50))
                          .WithSupervisorApiCycle(std::chrono::milliseconds(50))
                          .Build();
        if (!hm_res.has_value())
        {
            std::cerr << "Failed to build health monitor" << std::endl;
            return EXIT_FAILURE;
        }
        const auto hm = std::move(hm_res).value();

        auto deadline_monitor_res = hm->GetDeadlineMonitor(ident);
        if (!deadline_monitor_res.has_value())
        {
            std::cerr << "Failed to get deadline monitor" << std::endl;
            return EXIT_FAILURE;
        }
        const auto deadline_mon = std::move(deadline_monitor_res).value();

        if (!hm->Start().has_value())
        {
            std::cerr << "Failed to start health monitor" << std::endl;
            return EXIT_FAILURE;
        }

        score::mw::lifecycle::report_running();

        auto deadline_res = deadline_mon->GetDeadline(DeadlineTag{"deadline_1"});
        if (!deadline_res.has_value())
        {
            std::cerr << "Failed to get deadline" << std::endl;
            return EXIT_FAILURE;
        }
        const auto deadline = std::move(deadline_res).value();

        while (!exitRequested)
        {
            if (stopReportingCheckpoints.load())
            {
                break;
            }

            // The handle stops the deadline when it goes out of scope.
            const auto deadline_guard = deadline->Start();

            std::this_thread::sleep_for(std::chrono::milliseconds(config->delayInMs));
        }
    }

    while (stopReportingCheckpoints && !exitRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}

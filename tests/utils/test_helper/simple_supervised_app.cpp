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
/*
 * ./simple_supervised_app [--checkpointIntervalMs <ms>] --pidfile <path>
 *
 * Reports running, then reports an alive notification every <ms> milliseconds (default 100) until SIGTERM is received.
 * When SIGUSR1 is received the app stops reporting alive notifications but keeps running, which allows
 * tests to provoke an alive supervision failure.
 *
 */
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include "tests/utils/test_helper/arg_parser.hpp"
#include <score/mw/lifecycle/alive.h>
#include <score/mw/lifecycle/report_running.h>

namespace
{
using score::mw::lifecycle::test::ParseArgs;

/// @brief Used when --checkpointIntervalMs is not given.
constexpr std::chrono::milliseconds kDefaultCheckpointInterval{100};

struct CheckpointIntervalMsArg
{
    static constexpr std::string_view name = "--checkpointIntervalMs";
    using type = int;
    static constexpr type default_value = static_cast<int>(kDefaultCheckpointInterval.count());
};

struct PidfileArg
{
    static constexpr std::string_view name = "--pidfile";
    using type = std::string_view;
    static constexpr type default_value{};
};

std::atomic<bool> exit_requested{false};
std::atomic<bool> stop_reporting{false};

void signal_handler(int signal)
{
    if (signal == SIGTERM)
    {
        exit_requested = true;
    }
    else if (signal == SIGUSR1)
    {
        stop_reporting = true;
    }
}

struct Config
{
    std::chrono::milliseconds checkpoint_interval{kDefaultCheckpointInterval};
    std::string pidfile;
};

bool parse_options(int argc, char** argv, Config& config)
{
    const auto [checkpoint_interval_ms, pidfile] = ParseArgs<CheckpointIntervalMsArg, PidfileArg>(argc, argv);
    if (checkpoint_interval_ms <= 0)
    {
        return false;
    }
    config.checkpoint_interval = std::chrono::milliseconds{checkpoint_interval_ms};
    config.pidfile = std::string{pidfile};
    return !config.pidfile.empty();
}

bool write_pidfile(const std::string& path)
{
    std::ofstream file{path, std::ios::out | std::ios::trunc};
    file << ::getpid() << std::endl;
    return file.good();
}
}  // namespace

int main(int argc, char** argv)
{
    Config config;
    if (!parse_options(argc, argv, config))
    {
        std::cerr << "Usage: " << argv[0] << " [--checkpointIntervalMs <ms>] --pidfile <path>" << std::endl;
        return EXIT_FAILURE;
    }

    struct sigaction action{};
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGUSR1, &action, nullptr);

    if (!write_pidfile(config.pidfile))
    {
        std::cerr << "Failed to write pid file " << config.pidfile << std::endl;
        return EXIT_FAILURE;
    }

    score::mw::lifecycle::Alive alive{"simple_supervised_app"};
    score::mw::lifecycle::report_running();

    while (!exit_requested)
    {
        if (!stop_reporting)
        {
            alive.ReportAlive();
        }
        std::this_thread::sleep_for(config.checkpoint_interval);
    }

    return EXIT_SUCCESS;
}

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
#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

namespace
{
/// @brief Component name
std::string component_name{""};

/// @brief Flag signalling exit was requested by signal
bool exit_requested{false};

/// @return The component this binary is deployed as (PROCESSIDENTIFIER).
void store_component_name()
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    component_name = process_id ? std::string{process_id} : std::string{"gated_process"};
}

/// @brief Log a message to stdout, including the component name
void log(std::string msg)
{
    std::cout << "[" << component_name << "]: " << msg << "\n";
}

/// @brief Signal handler to guarantee cleanup file creation on signal termination
void signal_handler(int)
{
    exit_requested = true;
}

void register_signal_handlers()
{
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
}
}  // namespace

/// @brief Creates gatedStartedPath() on start, then reports running only once the test
// creates gatedReleasePath(). This keeps the activation of its Run Target in
// progress for as long as the test needs. If the launch manager stops it before it
// is released, it exits without reporting running.
// On exit, we create gatedExitingPath(), so a test can wait for this process to exit.
int main()
{
    store_component_name();
    register_signal_handlers();

    log("Signal start");
    if (!touch_file(gatedStartedPath(component_name)))
    {
        log("Failed to touch start file");
        return EXIT_FAILURE;
    }

    log("Wait for release");
    while (!exit_requested && !std::filesystem::exists(gatedReleasePath(component_name)))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (exit_requested)
    {
        log("Exit requested whilst waiting for release");
    }
    else
    {
        log("Report running");
        score::mw::lifecycle::report_running();

        log("Wait for termination");
        while (!exit_requested)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    log("Signal exiting");
    if (!touch_file(gatedExitingPath(component_name)))
    {
        log("Failed to touch exiting file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

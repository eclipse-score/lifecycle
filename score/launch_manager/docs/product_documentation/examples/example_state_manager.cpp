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
#include <cassert>
#include <chrono>
#include <csignal>
#include <iostream>

#include "score/mw/com/runtime.h"
#include "score/string_manipulation/arguments/arguments.h"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace std::literals::chrono_literals;

/// Used to signal if the startup run target switch was succesfull.
std::atomic<bool> startup_finished{false};

/// Used if a termination was requested from a signal.
std::atomic<bool> exitRequested{false};

void signalHandler(int signal)
{
    exitRequested = true;
}

int main(int argc, const char* argv[])
{
    // initialize com, needed if you are giving the mw_conf.json as an argument
    score::mw::com::runtime::InitializeRuntime(score::string_manipulation::GetArguments(argc, argv));

    // register signal handler and assert success
    assert(signal(SIGTERM, signalHandler));

    // report running to LM
    score::mw::lifecycle::report_running();

    // create a client
    auto client_result = score::mw::lifecycle::ILmControl::Create("StateManager/LaunchManager/Instance");
    if (!client_result.has_value())
    {
        std::cerr << "Failed to create client " << client_result.error() << std::endl;
        return EXIT_FAILURE;
    }

    // get the client from the result
    std::unique_ptr<score::mw::lifecycle::ILmControl> client = std::move(client_result).value();

    // register a method that gets called when a run target is switched
    const auto register_result = client->register_run_target_activation_callback(
        []([[maybe_unused]] score::mw::lifecycle::RunTargetActivationSource source,
           score::mw::lifecycle::RunTargetName target) {
            // signal that the startup was completed
            if (source == score::mw::lifecycle::RunTargetActivationSource::kInitialActivation)
            {
                std::cout << "Initial activation succesfull! activated run target " << target << std::endl;
                startup_finished.store(true);
            }
            else
            {
                std::cout << "Successfully activated Run Target " << target << std::endl;
            }
        });
    if (!register_result.has_value())
    {
        std::cerr << "Failed to register activation callback " << register_result.error() << std::endl;
        return EXIT_FAILURE;
    }

    while (startup_finished.load())
    {
        std::this_thread::sleep_for(100ms);
    }

    // activate a different run target
    const auto result = client->activate_run_target(score::mw::lifecycle::RunTargetName{"Running"}, true);
    if (result.has_value())
    {
        std::cout << "Successfully requested Run Target " << std::endl;
    }
    else
    {
        std::cerr << "Requesting Run Target "
                  << " failed with error: " << result.error().Message() << std::endl;
    }

    // wait until lm wants to terminate
    while (!exitRequested)
    {
        pause();
        std::this_thread::sleep_for(100ms);
    }

    return EXIT_SUCCESS;
}

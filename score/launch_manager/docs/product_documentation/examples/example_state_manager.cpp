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

#include <atomic>
#include <cassert>
#include <csignal>
#include <iostream>
#include <unistd.h>

#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

std::atomic<bool> exitRequested{false};
void signalHandler(int signal)
{
    exitRequested = true;
}

int main()
{
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
            std::cout << "Successfully activated Run Target " << target << std::endl;
        });
    if (!register_result.has_value())
    {
        std::cerr << "Failed to register activation callback " << register_result.error() << std::endl;
        return EXIT_FAILURE;
    }

    // activate a different run target
    const auto result = client->activate_run_target(score::mw::lifecycle::RunTargetName{"Running"}, true);
    if (result.has_value())
    {
        std::cerr << "Successfully requested Run Target " << std::endl;
    }
    else
    {
        std::cerr << "Requesting Run Target "
                  << " failed with error: " << result.error().Message() << std::endl;
    }

    // wait until lm wants to terminate
    while (exitRequested == 0)
    {
        pause();
    }

    return EXIT_SUCCESS;
}

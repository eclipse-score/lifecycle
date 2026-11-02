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
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "common.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>

using namespace score::mw::lifecycle;

namespace
{
constexpr auto kTimeout = std::chrono::seconds(20);
constexpr std::uint64_t kMaxFailures = 500;
}  // namespace

/// @brief Keeps sending LmControl requests (get_active_run_target and an "Off" activation) to the
/// Launch Manager once run_target_a is active, until the Launch Manager stops answering.
/// It is not owned by the Launch Manager, so it keeps sending while the Launch Manager shuts
/// down, including after the control client has been terminated and while the
/// ProcessGroupManager is being destroyed.
int main()
{
    while (!std::filesystem::exists(a_started))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const auto deadline = std::chrono::steady_clock::now() + kTimeout;
    std::uint64_t connect_attempt_count = 0U;
    std::uint64_t failure_count = 0U;
    std::uint64_t request_count = 0U;

    {  // client scope
        std::unique_ptr<ILmControl> client;

        while (std::chrono::steady_clock::now() < deadline && failure_count < kMaxFailures)
        {
            if (!client)
            {
                std::cout << "Connecting client\n";
                auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
                ++connect_attempt_count;

                if (!client_result.has_value())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                std::cout << "Client connected after " << connect_attempt_count << " attempts\n";
                client = std::move(client_result).value();
            }

            const bool active_ok = client->get_active_run_target().has_value();
            const bool activation_ok = client->activate_run_target("Off", true).has_value();
            request_count += 2;

            if (!(active_ok && activation_ok))
            {
                failure_count++;
            }
        }

        std::cout << "sent " << request_count << " requests\n";
        std::cout << "destroying client\n";
    }

    std::cout << "client destroyed, exiting\n";
    return 0;
}

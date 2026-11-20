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
#ifndef TESTS_UTILS_FIXTURES_CONTROL_CLIENT_HPP
#define TESTS_UTILS_FIXTURES_CONTROL_CLIENT_HPP

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <gtest/gtest.h>
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

/// @brief Fixture for initializing the control client handle
class ControlClientFixture : public ::testing::Test
{
  protected:
    void InitializeClient()
    {
        using namespace score::mw::lifecycle;

        TEST_STEP("Create client")
        {
            auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
            ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
            client = std::move(client_result).value();
        }

        TEST_STEP("Register callback")
        {
            const auto result = client->register_run_target_activation_callback(push_event);
            ASSERT_TRUE(result.has_value());
        }

        TEST_STEP("Wait for client connection")
        {
            ASSERT_TRUE(client->wait_for_connection().has_value());
        }

        TEST_STEP("Report running")
        {
            report_running();
        }
    }

    std::unique_ptr<score::mw::lifecycle::ILmControl> client;
};

#endif  // TESTS_UTILS_FIXTURES_CONTROL_CLIENT_HPP

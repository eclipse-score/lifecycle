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
    score::Result<std::unique_ptr<score::mw::lifecycle::ILmControl>> InitializeClient()
    {
        using namespace score::mw::lifecycle;
        std::unique_ptr<score::mw::lifecycle::ILmControl> client;

        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        if (!client_result.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<score::mw::lifecycle::ILmControl>>(client_result.error());
        }
        client = std::move(client_result).value();

        const auto register_result = client->register_run_target_activation_callback(push_event);
        if (!register_result.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<score::mw::lifecycle::ILmControl>>(register_result.error());
        }

        auto connection_result = client->wait_for_connection();
        if (!connection_result.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<score::mw::lifecycle::ILmControl>>(connection_result.error());
        }

        return client;
    }
};

#endif  // TESTS_UTILS_FIXTURES_CONTROL_CLIENT_HPP

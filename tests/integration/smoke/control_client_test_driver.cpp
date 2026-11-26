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
#include <gtest/gtest.h>
#include <unistd.h>
#include <csignal>
#include <iostream>

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

using Smoke = ActivationFixture;

TEST_F(Smoke, Daemon)
{
    std::unique_ptr<ILmControl> client;

    TEST_STEP("Initialize client")
    {
        auto maybe_client = InitializeClient();
        ASSERT_TRUE(maybe_client.has_value()) << maybe_client.error().Message();
        client = std::move(maybe_client).value();
    }

    TEST_STEP("Validate active Run Target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_FALSE(result.has_value()) << "Should not be active until we report running";
        EXPECT_EQ(result.error(), ExecErrc::kActivationInProgress);
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Callback for Run Target Startup")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Validate active Run Target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }

    TEST_STEP("Activate Run Target Running")
    {
        const auto result = client->activate_run_target("Running", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for Run Target Running")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "Running");
    }

    TEST_STEP("Validate active Run Target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Running");
    }

    TEST_STEP("Activate Run Target Startup")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for Run Target Startup")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "Startup");
    }

    TEST_STEP("Validate active Run Target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }
}

int main()
{
    TestRunner::Init();
    const auto test_result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return test_result;
}

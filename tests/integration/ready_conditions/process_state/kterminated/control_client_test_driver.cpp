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
#include <gtest/gtest.h>

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

using ProcessState = ActivationFixture;

TEST_F(ProcessState, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({fallback_file}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Initialize client")
    {
        auto maybe_client = InitializeClient();
        ASSERT_TRUE(maybe_client.has_value()) << maybe_client.error().Message();
        client = std::move(maybe_client).value();
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Callback for RunTarget Startup")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Activate RunTarget that works")
    {
        const auto result = client->activate_run_target("working", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for RunTarget working")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "working");
    }

    TEST_STEP("Activate RunTarget that times out")
    {
        const auto result = client->activate_run_target("timeout", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for RunTarget fallback")
    {
        WaitForActivation(RunTargetActivationSource::kRecoveryAction, "fallback_run_target");
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return result;
}

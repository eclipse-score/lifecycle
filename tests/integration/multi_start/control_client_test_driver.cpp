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
#include <memory>

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST_F(ActivationFixture, GivenThreeComponentsWithTheSameExecutable_WhenTheirRunTargetIsActivated_ThenAllInstancesRun)
{
    std::unique_ptr<ILmControl> client;

    TEST_STEP("Initialize client")
    {
        auto maybe_client = InitializeClient();
        ASSERT_TRUE(maybe_client.has_value());
        client = std::move(maybe_client).value();
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Wait for the initial run target to start")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Activate Run Target MultiStart")
    {
        const auto result = client->activate_run_target("MultiStart", true);
        ASSERT_TRUE(result.has_value()) << result.error().Message();
    }

    // Each instance is a Reporting component, so the Run Target is only reached when all three processes of the
    // same executable run at the same time and each of them reported running.
    TEST_STEP("Callback for Run Target MultiStart")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "MultiStart");
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

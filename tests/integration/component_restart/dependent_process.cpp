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

#include <filesystem>

#include "tests/integration/component_restart/component_restart_files.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

TEST(ComponentRestart, DependentProcess)
{
    TEST_STEP("Check this is the first start")
    {
        ASSERT_TRUE(check_clean({kDependentStartedFile, kRecoveryVerifiedFile}))
            << "Dependent component was started more than once!";
        ASSERT_TRUE(touch_file(kDependentStartedFile));
    }

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TestRunner::WaitForTermination();

    TEST_STEP("Verify this component kept running during the recovery of its dependencies")
    {
        EXPECT_TRUE(std::filesystem::exists(kRecoveryVerifiedFile))
            << "Dependent component was stopped before the recovery of its dependencies was verified";
    }
}

int main()
{
    TestRunner::Init();
    return TestRunner::RunTests();
}

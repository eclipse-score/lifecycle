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

#include <unistd.h>

#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

// Given a correct configuration with:
//   - An initial Run Target named "Startup"
//   - Startup contains one Component named "component_logging"

// When launch manager is started

TEST(ComponentActivationLog, ProcessLogging)
{
    // Then, the process is started and:
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
    TEST_STEP("Record own pid")
    {
        // The python test compares this with the pid in the launch manager's activation log.
        RecordProperty("pid", static_cast<int>(getpid()));
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    return result;
}

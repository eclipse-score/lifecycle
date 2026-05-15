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
#include <csignal>
#include <iostream>
#include <unistd.h>
#include <gtest/gtest.h>

#include <score/lcm/lifecycle_client.h>
#include <score/lcm/control_client.h>
#include <score/lcm/identifier_hash.hpp>
#include "tests/utils/test_helper/test_helper.hpp"

score::lcm::ControlClient client;

TEST(Smoke, Daemon) {
    TEST_STEP("Control daemon report kRunning") {
        // report kRunning
        auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);
        ASSERT_TRUE(result.has_value()) << "client.ReportExecutionState() failed: " << result.error().Message();
    }
    TEST_STEP("Activate RunTarget Running") {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Running").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target Running failed: " << result.error().Message();
    }
    TEST_STEP("Activate RunTarget Startup") {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        EXPECT_TRUE(result.has_value());
    }
    TEST_STEP("Activate RunTarget Off") {
        client.ActivateRunTarget("Off");
    }
}

int main(int argc, char** argv) {
    return TestRunner(__FILE__, true).RunTests();
}

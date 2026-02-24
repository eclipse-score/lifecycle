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
#include "tests/integration/test_helper.hpp"

score::lcm::ControlClient client([](const score::lcm::ExecutionErrorEvent& event) {
    std::cerr << "Undefined state callback invoked for process group id: " << event.processGroup.data() << std::endl;
});

// create DefaultPG
const score::lcm::IdentifierHash defaultpg {"DefaultPG"};
const score::lcm::IdentifierHash defaultpgOn {"DefaultPG/On"};
const score::lcm::IdentifierHash defaultpgOff {"DefaultPG/Off"};
// MainPG
const score::lcm::IdentifierHash mainpg {"MainPG"};
const score::lcm::IdentifierHash mainpgOff {"MainPG/Off"};

TEST(Smoke, Daemon) {
    TEST_STEP("Control daemon report kRunning") {
        // report kRunning
        auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);

        ASSERT_TRUE(result.has_value()) << "client.ReportExecutionState() failed";
    }
    TEST_STEP("Turn default PG on") {
        score::cpp::stop_token stop_token;
        auto result = client.SetState(defaultpg, defaultpgOn).Get(stop_token);
        EXPECT_TRUE(result.has_value());
    }
    TEST_STEP("Turn default PG off") {
        score::cpp::stop_token stop_token;
        auto result = client.SetState(defaultpg, defaultpgOff).Get(stop_token);
        EXPECT_TRUE(result.has_value());
    }
    TEST_STEP("Turn main PG off") {
        client.SetState(mainpg, mainpgOff);
    }
}

int main(int argc, char** argv) {
    return TestRunner(__FILE__, true).RunTests();
}

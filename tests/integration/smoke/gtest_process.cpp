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

#include <score/lcm/lifecycle_client.h>
#include <score/lcm/control_client.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <csignal>
#include "tests/integration/test_helper.hpp"

TEST(Smoke, Process) {
    // report kRunning
    auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);

    ASSERT_TRUE(result.has_value()) << "client.ReportExecutionState() failed";
}

int main() {
    return TestRunner(__FILE__).RunTests();
}

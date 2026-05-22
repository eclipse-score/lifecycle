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
#include <cstdlib>
#include <string>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/lifecycle_client.h>

TEST(ReportingProcess, ReportsRunning)
{
    auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
    EXPECT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
}

int main()
{
    // Prevent XML file naming collisions of different components running this binary
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    const std::string xml_name = process_id ? (std::string{"reporting_process_"} + process_id) : __FILE__;
    return TestRunner(xml_name).RunTests();
}

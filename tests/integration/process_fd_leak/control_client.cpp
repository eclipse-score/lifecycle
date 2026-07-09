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

#include <sstream>

#include "get_fds.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

int g_argc;
char** g_argv;

TEST(ProcessFDs, ProcessInitial)
{

    score::mw::lifecycle::report_running();

    score::mw::lifecycle::ControlClient client{};

    auto open_fds = get_fds();
    std::ostringstream oss;
    oss << open_fds;
    EXPECT_TRUE(open_fds.size() == 0) << "Found open files!\n" << oss.str();
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;

    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd};

    return runner.RunTests();
}

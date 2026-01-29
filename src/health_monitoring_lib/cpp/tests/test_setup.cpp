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
#include "score/mw/log/rust/stdout_logger_init.h"
#include <gtest/gtest.h>
#include <iostream>

class LogEnvironment : public ::testing::Environment
{
  public:
    void SetUp() override
    {
        using namespace score::mw::log::rust;

        StdoutLoggerBuilder builder;
        builder.Context("TEST").LogLevel(LogLevel::Verbose).SetAsDefaultLogger();
    }

    void TearDown() override
    {
        // Runs once after all tests
    }
};

struct RegisterLogEnvironment
{
    RegisterLogEnvironment()
    {
        ::testing::AddGlobalTestEnvironment(new LogEnvironment());
    }
};

static RegisterLogEnvironment s_register_log_environment;

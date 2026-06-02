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

/// Initializer for `StdoutLogger`.
/// This enables logging for Rust libraries.
class LogEnvironment final : public ::testing::Environment
{
  protected:
    void SetUp() override
    {
        using namespace score::mw::log::rust;

        StdoutLoggerBuilder builder;
        builder.Context("TEST")
            .LogLevel(LogLevel::Verbose)
            .ShowModule(false)
            .ShowFile(true)
            .ShowLine(true)
            .SetAsDefaultLogger();
    }
};

struct RegisterLogEnvironment
{
    RegisterLogEnvironment()
    {
        ::testing::AddGlobalTestEnvironment(new LogEnvironment());
    }
};

static RegisterLogEnvironment register_log_environment;

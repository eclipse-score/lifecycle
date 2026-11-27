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

#include "score/health_monitor/error.h"

#include "score/result/result.h"

#include <gtest/gtest.h>

#include <string_view>

using namespace score::health_monitor;

TEST(ErrorCodeTest, IntegerValuesAreStable)
{
    EXPECT_EQ(static_cast<int>(ErrorCode::kUnknownTag), 1);
    EXPECT_EQ(static_cast<int>(ErrorCode::kAlreadyTaken), 2);
    EXPECT_EQ(static_cast<int>(ErrorCode::kNotTaken), 3);
    EXPECT_EQ(static_cast<int>(ErrorCode::kAlreadyStarted), 4);
    EXPECT_EQ(static_cast<int>(ErrorCode::kNoMonitors), 5);
    EXPECT_EQ(static_cast<int>(ErrorCode::kInvalidArgument), 6);
    EXPECT_EQ(static_cast<int>(ErrorCode::kDeadlineRunning), 7);
    EXPECT_EQ(static_cast<int>(ErrorCode::kDeadlineFailed), 8);
    EXPECT_EQ(static_cast<int>(ErrorCode::kInvalidTransition), 9);
    EXPECT_EQ(static_cast<int>(ErrorCode::kInvalidState), 10);
}

TEST(ErrorDomainTest, MessageForEveryCode)
{
    const ErrorDomain domain{};
    const auto message{[&domain](const ErrorCode code) {
        return domain.MessageFor(static_cast<score::result::ErrorCode>(code));
    }};

    EXPECT_EQ(message(ErrorCode::kUnknownTag), "Unknown tag");
    EXPECT_EQ(message(ErrorCode::kAlreadyTaken), "Already taken");
    EXPECT_EQ(message(ErrorCode::kNotTaken), "Monitor not taken");
    EXPECT_EQ(message(ErrorCode::kAlreadyStarted), "Health monitor already started");
    EXPECT_EQ(message(ErrorCode::kNoMonitors), "No monitors");
    EXPECT_EQ(message(ErrorCode::kInvalidArgument), "Invalid argument");
    EXPECT_EQ(message(ErrorCode::kDeadlineRunning), "Deadline already running");
    EXPECT_EQ(message(ErrorCode::kDeadlineFailed), "Deadline failed");
    EXPECT_EQ(message(ErrorCode::kInvalidTransition), "Invalid transition");
    EXPECT_EQ(message(ErrorCode::kInvalidState), "Invalid state");
}

TEST(ErrorDomainTest, UnmappedValuesGiveUnknownError)
{
    const ErrorDomain domain{};

    EXPECT_EQ(domain.MessageFor(0), "Unknown error");
    EXPECT_EQ(domain.MessageFor(11), "Unknown error");
    EXPECT_EQ(domain.MessageFor(-1), "Unknown error");
}

TEST(MakeErrorTest, KeepsCodeAndMessage)
{
    const score::result::Error error{MakeError(ErrorCode::kNotTaken)};

    EXPECT_EQ(*error, static_cast<score::result::ErrorCode>(ErrorCode::kNotTaken));
    EXPECT_EQ(error.Message(), "Monitor not taken");
    EXPECT_TRUE(error.UserMessage().empty());
}

TEST(MakeErrorTest, KeepsUserMessage)
{
    const score::result::Error error{MakeError(ErrorCode::kInvalidArgument, "cycle is zero")};

    EXPECT_EQ(error.Message(), "Invalid argument");
    EXPECT_EQ(error.UserMessage(), "cycle is zero");
}

TEST(MakeErrorTest, MakeUnexpectedFindsMakeErrorThroughAdl)
{
    const score::Result<int> result{score::MakeUnexpected(ErrorCode::kAlreadyStarted)};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(*result.error(), static_cast<score::result::ErrorCode>(ErrorCode::kAlreadyStarted));
    EXPECT_EQ(result.error().Message(), "Health monitor already started");
}

TEST(MakeErrorTest, ErrorsOfDifferentCodesAreUnequal)
{
    EXPECT_EQ(MakeError(ErrorCode::kNoMonitors), MakeError(ErrorCode::kNoMonitors));
    EXPECT_NE(MakeError(ErrorCode::kNoMonitors), MakeError(ErrorCode::kNotTaken));
}

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

#include "score/mw/launch_manager/watchdog/critical_error.hpp"

#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal::watchdog
{
namespace
{

TEST(CriticalErrorTest, ToString_ReturnsNameOfErrorType)
{
    RecordProperty("Description", "toString() returns the name of each CriticalErrorType value.");
    EXPECT_EQ(toString(CriticalErrorType::kGeneralError), "GeneralError");
    EXPECT_EQ(toString(CriticalErrorType::kQueueOverflow), "QueueOverflow");
    EXPECT_EQ(toString(CriticalErrorType::kThreadAliveCheckFailed), "ThreadAliveCheckFailed");
}

TEST(CriticalErrorTest, ToString_ReturnsUnknownForOutOfRangeValue)
{
    RecordProperty("Description", "toString() returns \"Unknown\" for a value outside of CriticalErrorType.");
    EXPECT_EQ(toString(static_cast<CriticalErrorType>(255U)), "Unknown");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::watchdog

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

#include "gtest/gtest.h"
#include "score/hm/common.h"
// #include <gtest/gtest.h>

using namespace score::hm;

TEST(TimeRange, New_Succeeds)
{
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};
}

TEST(TimeRange, New_InvalidOrder)
{
    using namespace std::chrono_literals;
    // `SIGABRT` is expected.
    ASSERT_DEATH({ TimeRange range(200ms, 100ms); }, "");
}

TEST(TimeRange, MinMax)
{
    using namespace std::chrono_literals;
    TimeRange range{123ms, 456ms};
    ASSERT_EQ(range.min_ms(), 123);
    ASSERT_EQ(range.max_ms(), 456);
}

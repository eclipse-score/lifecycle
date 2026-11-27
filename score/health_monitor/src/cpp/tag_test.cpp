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

#include "score/health_monitor/tag.h"

#include <gtest/gtest.h>

#include <type_traits>

using namespace score::health_monitor;

namespace
{

constexpr MonitorTag kConstexprTag{"abc"};

}  // namespace

static_assert(kConstexprTag.Size() == 3U);
static_assert(kConstexprTag == MonitorTag{"abc"});
static_assert(kConstexprTag != MonitorTag{"abd"});

static_assert(!std::is_same_v<MonitorTag, DeadlineTag>);
static_assert(!std::is_same_v<MonitorTag, StateTag>);
static_assert(!std::is_same_v<DeadlineTag, StateTag>);
static_assert(!std::is_convertible_v<MonitorTag, DeadlineTag>);
static_assert(!std::is_convertible_v<DeadlineTag, StateTag>);
static_assert(!std::is_convertible_v<StateTag, MonitorTag>);

TEST(TagTest, EqualityComparesTextNotPointer)
{
    const char first[]{"same"};
    const char second[]{"same"};
    ASSERT_NE(static_cast<const void*>(first), static_cast<const void*>(second));

    EXPECT_TRUE(MonitorTag{first} == MonitorTag{second});
    EXPECT_FALSE(MonitorTag{first} != MonitorTag{second});
}

TEST(TagTest, DifferentTextIsUnequal)
{
    EXPECT_TRUE(MonitorTag{"one"} != MonitorTag{"two"});
    EXPECT_FALSE(MonitorTag{"one"} == MonitorTag{"two"});
}

TEST(TagTest, PrefixIsUnequal)
{
    EXPECT_TRUE(MonitorTag{"abc"} != MonitorTag{"abcd"});
}

TEST(TagTest, SizeExcludesTerminatingNull)
{
    const MonitorTag tag{"hello"};

    EXPECT_EQ(tag.Size(), 5U);
    EXPECT_EQ(tag.Data()[tag.Size()], '\0');
}

TEST(TagTest, EmptyTextHasSizeZero)
{
    const MonitorTag tag{""};

    EXPECT_EQ(tag.Size(), 0U);
    EXPECT_TRUE(tag == MonitorTag{""});
}

TEST(TagTest, DataPointsToText)
{
    const DeadlineTag tag{"deadline"};

    EXPECT_STREQ(tag.Data(), "deadline");
}

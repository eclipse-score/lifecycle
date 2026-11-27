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
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that two tags with equal text at different addresses are equal.");

    const char first[]{"same"};
    const char second[]{"same"};
    ASSERT_NE(static_cast<const void*>(first), static_cast<const void*>(second));

    EXPECT_TRUE(MonitorTag{first} == MonitorTag{second});
    EXPECT_FALSE(MonitorTag{first} != MonitorTag{second});
}

TEST(TagTest, DifferentTextIsUnequal)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that tags with different text are unequal.");

    EXPECT_TRUE(MonitorTag{"one"} != MonitorTag{"two"});
    EXPECT_FALSE(MonitorTag{"one"} == MonitorTag{"two"});
}

TEST(TagTest, PrefixIsUnequal)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a tag is unequal to a tag whose text extends it by one character.");

    EXPECT_TRUE(MonitorTag{"abc"} != MonitorTag{"abcd"});
}

TEST(TagTest, SizeExcludesTerminatingNull)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the size of a tag counts its characters without the terminating null.");

    const MonitorTag tag{"hello"};

    EXPECT_EQ(tag.Size(), 5U);
    EXPECT_EQ(tag.Data()[tag.Size()], '\0');
}

TEST(TagTest, EmptyTextHasSizeZero)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a tag with empty text has size zero and equals another empty tag.");

    const MonitorTag tag{""};

    EXPECT_EQ(tag.Size(), 0U);
    EXPECT_TRUE(tag == MonitorTag{""});
}

TEST(TagTest, DataPointsToText)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__monitor_id, comp_req__health_monitor__deadline_id, "
        "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the data of a tag is its null-terminated text.");

    const DeadlineTag tag{"deadline"};

    EXPECT_STREQ(tag.Data(), "deadline");
}

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

#include "score/health_monitor/thread_parameters.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace score::health_monitor;

TEST(ThreadParametersTest, UnsetValuesAreEmpty)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that a default ThreadParameters has no scheduler parameters, affinity or stack size.");

    const ThreadParameters parameters{};

    EXPECT_FALSE(parameters.GetSchedulerParameters().has_value());
    EXPECT_TRUE(parameters.GetAffinity().empty());
    EXPECT_FALSE(parameters.GetStackSize().has_value());
}

TEST(ThreadParametersTest, SchedulerParametersAreKept)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the getter returns the policy and priority that were set.");

    const ThreadParameters parameters{ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, 42})};

    const auto scheduler = parameters.GetSchedulerParameters();
    ASSERT_TRUE(scheduler.has_value());
    EXPECT_EQ(scheduler->Policy(), SchedulerPolicy::kFifo);
    EXPECT_EQ(scheduler->Priority(), 42);
}

TEST(ThreadParametersTest, StackSizeIsKept)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the getter returns the stack size that was set.");

    const ThreadParameters parameters{ThreadParameters{}.WithStackSize(65536U)};

    ASSERT_TRUE(parameters.GetStackSize().has_value());
    EXPECT_EQ(*parameters.GetStackSize(), 65536U);
}

TEST(ThreadParametersTest, AffinityIsKept)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the getter returns the CPU indices that were set in order.");

    const std::array<std::size_t, 3> cpus{{2U, 0U, 1U}};
    const ThreadParameters parameters{ThreadParameters{}.WithAffinity(cpus)};

    const auto affinity = parameters.GetAffinity();
    ASSERT_EQ(affinity.size(), 3U);
    EXPECT_EQ(affinity[0], 2U);
    EXPECT_EQ(affinity[1], 0U);
    EXPECT_EQ(affinity[2], 1U);
}

TEST(ThreadParametersTest, AffinityIsACopyOfTheArgument)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that changing or destroying the argument span does not change the stored affinity.");

    ThreadParameters parameters{};
    {
        std::vector<std::size_t> cpus{0U, 1U};
        parameters = ThreadParameters{}.WithAffinity(cpus);
        cpus[0] = 7U;
        cpus.clear();
    }

    const auto affinity = parameters.GetAffinity();
    ASSERT_EQ(affinity.size(), 2U);
    EXPECT_EQ(affinity[0], 0U);
    EXPECT_EQ(affinity[1], 1U);
}

TEST(ThreadParametersTest, SettersCombine)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that setting one value does not reset another.");

    const std::array<std::size_t, 1> cpus{{0U}};
    const ThreadParameters parameters{
        ThreadParameters{}.WithStackSize(65536U).WithAffinity(cpus).WithSchedulerParameters(
            {SchedulerPolicy::kRoundRobin, 5})};

    EXPECT_EQ(parameters.GetStackSize(), 65536U);
    EXPECT_EQ(parameters.GetAffinity().size(), 1U);
    ASSERT_TRUE(parameters.GetSchedulerParameters().has_value());
    EXPECT_EQ(parameters.GetSchedulerParameters()->Policy(), SchedulerPolicy::kRoundRobin);
}

TEST(SchedulerPolicyPriorityTest, FifoMinimumIsOne)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the lowest FIFO priority is 1.");

    EXPECT_EQ(SchedulerPolicyPriorityMin(SchedulerPolicy::kFifo), 1);
}

TEST(SchedulerPolicyPriorityTest, FifoMaximumMatchesPlatform)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the highest FIFO priority is 99 on Linux and 253 on QNX.");

#if defined(__QNX__)
    EXPECT_EQ(SchedulerPolicyPriorityMax(SchedulerPolicy::kFifo), 253);
#else
    EXPECT_EQ(SchedulerPolicyPriorityMax(SchedulerPolicy::kFifo), 99);
#endif
}

TEST(SchedulerPolicyPriorityTest, MinimumDoesNotExceedMaximumForEveryPolicy)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that the minimum priority does not exceed the maximum for every policy.");

    for (const SchedulerPolicy policy : {SchedulerPolicy::kOther, SchedulerPolicy::kFifo, SchedulerPolicy::kRoundRobin})
    {
        EXPECT_LE(SchedulerPolicyPriorityMin(policy), SchedulerPolicyPriorityMax(policy));
    }
}

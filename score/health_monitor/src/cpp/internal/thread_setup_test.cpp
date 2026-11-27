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

#include "score/health_monitor/src/cpp/internal/thread_setup.h"

#include "score/health_monitor/thread_parameters.h"

#include <gtest/gtest.h>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <cstddef>
#include <thread>
#include <vector>

using namespace score::health_monitor;
using namespace score::health_monitor::details;

TEST(ApplyThreadParametersTest, AppliesSchedulerPolicyAndPriorityToTheCallingThread)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that the calling thread runs with SCHED_FIFO and the given priority afterwards.");

    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "Setting a real-time policy needs root.";
    }
    const int priority{SchedulerPolicyPriorityMin(SchedulerPolicy::kFifo)};
    int policy{-1};
    sched_param param{};

    // A helper thread keeps the policy change away from the test thread.
    std::thread helper{[&] {
        ApplyThreadParameters(ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, priority}));
        ::pthread_getschedparam(::pthread_self(), &policy, &param);
    }};
    helper.join();

    EXPECT_EQ(policy, SCHED_FIFO);
    EXPECT_EQ(param.sched_priority, priority);
}

TEST(ApplyThreadParametersTest, TerminatesTheProcessWhenTheCpuDoesNotExist)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that applying an affinity to a CPU index the system does not have aborts the process.");

    const long cpu_count{::sysconf(_SC_NPROCESSORS_ONLN)};
    ASSERT_GT(cpu_count, 0);
    const std::vector<std::size_t> cpus{static_cast<std::size_t>(cpu_count)};

    EXPECT_DEATH(ApplyThreadParameters(ThreadParameters{}.WithAffinity(cpus)), "");
}

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

#include "score/mw/launch_manager/process_group_manager/details/completion_slot.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <new>
#include <thread>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/result/result.h"

namespace
{

thread_local bool g_tracking_enabled = false;
thread_local std::size_t g_tracked_allocations = 0;

}  // namespace

void* operator new(std::size_t size)
{
    if (g_tracking_enabled)
    {
        ++g_tracked_allocations;
    }
    void* ptr = std::malloc(size);
    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

namespace score::mw::lifecycle::internal
{
namespace
{

class CompletionSlotTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "requirements-based");
    }
};

TEST_F(CompletionSlotTest, SynchronousCompleteThenWaitReturnsValue)
{
    RecordProperty("Description", "Verify CompletionSlot returns the published value synchronously.");

    CompletionSlot<int> slot;
    slot.complete(42);

    auto result = slot.wait();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(CompletionSlotTest, SynchronousCompleteResultVoid)
{
    RecordProperty("Description", "Verify CompletionSlot works with score::Result<void>.");

    CompletionSlot<score::Result<void>> slot;
    slot.complete(score::Result<void>{});

    auto result = slot.wait();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().has_value());
}

TEST_F(CompletionSlotTest, AsynchronousCompleteFromWorkerThread)
{
    RecordProperty("Description", "Verify CompletionSlot unblocks waiting thread when completed asynchronously.");

    CompletionSlot<score::Result<IdentifierHash>> slot;
    const IdentifierHash expected_hash{"TargetRunState"};

    std::thread worker([&slot, expected_hash]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        slot.complete(score::Result<IdentifierHash>{expected_hash});
    });

    auto result = slot.wait();
    worker.join();

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value().has_value());
    EXPECT_EQ(result.value().value(), expected_hash);
}

TEST_F(CompletionSlotTest, PreCancelledStopTokenAbortsWait)
{
    RecordProperty("Description", "Verify CompletionSlot::wait returns nullopt if stop_token is already stopped.");

    CompletionSlot<int> slot;
    score::cpp::stop_source source;
    source.request_stop();

    auto result = slot.wait(source.get_token());
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompletionSlotTest, ZeroAllocationInCompleteAndWait)
{
    RecordProperty("Description", "Verify CompletionSlot complete() and wait() perform zero dynamic memory allocations.");

    const IdentifierHash test_hash{"Running"};

    g_tracked_allocations = 0;
    g_tracking_enabled = true;

    {
        CompletionSlot<score::Result<IdentifierHash>> slot;
        slot.complete(score::Result<IdentifierHash>{test_hash});
        auto result = slot.wait();
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result.value().has_value());
        EXPECT_EQ(result.value().value(), test_hash);
    }

    g_tracking_enabled = false;
    EXPECT_EQ(g_tracked_allocations, 0U);
}

TEST_F(CompletionSlotTest, ZeroAllocationInResultVoidCompleteAndWait)
{
    RecordProperty("Description", "Verify CompletionSlot with Result<void> performs zero dynamic memory allocations.");

    g_tracked_allocations = 0;
    g_tracking_enabled = true;

    {
        CompletionSlot<score::Result<void>> slot;
        slot.complete(score::Result<void>{});
        auto result = slot.wait();
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result.value().has_value());
    }

    g_tracking_enabled = false;
    EXPECT_EQ(g_tracked_allocations, 0U);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

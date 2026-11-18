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
#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_event_queue.hpp"
#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

// The underlying MpscBoundedQueue is tested in mpsc_bounded_queue_test.cpp; these tests cover
// only what SupervisionEventQueue adds: the sticky overflow flag.

using namespace score::mw::lifecycle;

namespace
{

const IdentifierHash kProcess{"Process"};

SupervisionEvent makeEvent(std::int64_t seq)
{
    SupervisionEvent e{};
    e.id = kProcess;
    e.eventType = SupervisionEventType::kActivation;
    e.systemClockTimestamp.tv_nsec = static_cast<long>(seq);
    return e;
}

class SupervisionEventQueue_UT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
    }
};

}  // namespace

TEST_F(SupervisionEventQueue_UT, OverflowFlag_NotSetWithoutOverflow)
{
    RecordProperty(
        "Description", "The overflow flag is clear initially and stays clear while events are accepted and dequeued.");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    SupervisionEventQueue queue{2};
    EXPECT_FALSE(queue.getOverflowFlag());

    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0)));
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(1)));
    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_FALSE(queue.tryDequeue(out));
    EXPECT_FALSE(queue.getOverflowFlag());
}

TEST_F(SupervisionEventQueue_UT, OverflowFlag_SetWhenFull_AndNewestEventDropped)
{
    RecordProperty(
        "Description", "Enqueuing into a full queue fails, sets the overflow flag and drops the newest event.");
    RecordProperty("DerivationTechnique", "boundary-values");
    SupervisionEventQueue queue{2};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0)));
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(1)));
    EXPECT_FALSE(queue.tryEnqueue(makeEvent(2)));
    EXPECT_TRUE(queue.getOverflowFlag());

    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_EQ(out.systemClockTimestamp.tv_nsec, 0);
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_EQ(out.systemClockTimestamp.tv_nsec, 1);
    EXPECT_FALSE(queue.tryDequeue(out));
}

TEST_F(SupervisionEventQueue_UT, OverflowFlag_IsSticky_AndResettable)
{
    RecordProperty(
        "Description",
        "The overflow flag stays set across further queue operations until it is cleared by getOverflowFlag(true).");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    SupervisionEventQueue queue{1};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0)));
    ASSERT_FALSE(queue.tryEnqueue(makeEvent(1)));

    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(2)));

    EXPECT_TRUE(queue.getOverflowFlag());
    EXPECT_TRUE(queue.getOverflowFlag(false));
    EXPECT_TRUE(queue.getOverflowFlag(true));
    EXPECT_FALSE(queue.getOverflowFlag());
    EXPECT_FALSE(queue.getOverflowFlag(true));
}

TEST_F(SupervisionEventQueue_UT, DefaultCapacity_IsBufferQueueSize)
{
    RecordProperty(
        "Description",
        "A default-constructed queue holds BUFFER_QUEUE_SIZE events, then rejects further events and sets the overflow "
        "flag.");
    RecordProperty("DerivationTechnique", "boundary-values");
    SupervisionEventQueue queue;
    for (std::size_t i = 0; i < BufferConstants::BUFFER_QUEUE_SIZE; ++i)
    {
        ASSERT_TRUE(queue.tryEnqueue(makeEvent(static_cast<std::int64_t>(i))));
    }
    EXPECT_FALSE(queue.getOverflowFlag());
    EXPECT_FALSE(queue.tryEnqueue(makeEvent(0)));
    EXPECT_TRUE(queue.getOverflowFlag());
}

TEST_F(SupervisionEventQueue_UT, OverflowFlag_ConcurrentReset_NoLostSet)
{
    RecordProperty(
        "Description",
        "Concurrent overflow-flag resets and overflowing enqueues never lose a set: after the last reset the flag is "
        "set or a reset observed it.");
    RecordProperty("DerivationTechnique", "boundary-values");
    constexpr std::size_t kProducers = 16;
    SupervisionEventQueue queue{1};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0)));

    std::atomic<bool> stop{false};
    std::atomic<std::uint64_t> resetsSeenTrue{0};
    std::thread resetter{[&] {
        while (!stop.load(std::memory_order_acquire))
        {
            if (queue.getOverflowFlag(true))
            {
                resetsSeenTrue.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }};

    std::vector<std::thread> producers;
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        producers.emplace_back([&] {
            for (int i = 0; i < 1000; ++i)
            {
                (void)queue.tryEnqueue(makeEvent(1));  // always full: every call overflows
            }
        });
    }
    for (auto& t : producers)
    {
        t.join();
    }
    stop.store(true, std::memory_order_release);
    resetter.join();

    EXPECT_TRUE(queue.getOverflowFlag() || resetsSeenTrue.load() > 0);
}

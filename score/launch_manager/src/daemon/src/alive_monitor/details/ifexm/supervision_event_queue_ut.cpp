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
#include <memory>
#include <thread>
#include <vector>

using namespace score::mw::lifecycle;
using score::mw::lifecycle::internal::ConcurrencyErrc;

namespace score::mw::lifecycle
{
// Friend of SupervisionEventQueue: reaches kStopped mapping, which production code cannot trigger.
struct SupervisionEventQueueTestPeer
{
    static bool map(const score::Result<void>& r, std::atomic<bool>& flag)
    {
        return SupervisionEventQueue::mapEnqueueResult(r, flag);
    }
};
}  // namespace score::mw::lifecycle

namespace
{

const IdentifierHash kProcess{"Process"};

SupervisionEvent makeEvent(std::int64_t producer, std::int64_t seq)
{
    SupervisionEvent e{};
    e.id = kProcess;
    e.eventType = (seq % 2 == 0) ? SupervisionEventType::kActivation : SupervisionEventType::kDeactivation;
    e.systemClockTimestamp.tv_sec = static_cast<time_t>(producer);
    e.systemClockTimestamp.tv_nsec = static_cast<long>(seq);
    return e;
}

bool payloadIntact(const SupervisionEvent& e)
{
    const auto seq = static_cast<std::int64_t>(e.systemClockTimestamp.tv_nsec);
    const auto expected = (seq % 2 == 0) ? SupervisionEventType::kActivation : SupervisionEventType::kDeactivation;
    return e.id == kProcess && e.eventType == expected;
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

TEST_F(SupervisionEventQueue_UT, DefaultCapacity_IsBufferQueueSize)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    SupervisionEventQueue queue;
    for (std::size_t i = 0; i < BufferConstants::BUFFER_QUEUE_SIZE; ++i)
    {
        ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, static_cast<std::int64_t>(i))));
    }
    EXPECT_FALSE(queue.tryEnqueue(makeEvent(0, 0)));
    EXPECT_TRUE(queue.getOverflowFlag());
    SupervisionEvent out{};
    while (queue.tryDequeue(out))
    {
    }
}

TEST_F(SupervisionEventQueue_UT, EnqueueDequeue_SingleEvent_RoundTrips)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    SupervisionEventQueue queue{4};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(3, 7)));
    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_EQ(out.id, kProcess);
    EXPECT_EQ(out.systemClockTimestamp.tv_sec, 3);
    EXPECT_EQ(out.systemClockTimestamp.tv_nsec, 7);
    EXPECT_EQ(out.eventType, SupervisionEventType::kDeactivation);
    EXPECT_FALSE(queue.getOverflowFlag());
}

TEST_F(SupervisionEventQueue_UT, Dequeue_FromEmptyQueue_ReturnsFalse)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    SupervisionEventQueue queue{4};
    SupervisionEvent out{};
    EXPECT_FALSE(queue.tryDequeue(out));
    EXPECT_FALSE(queue.getOverflowFlag());
}

TEST_F(SupervisionEventQueue_UT, Dequeue_MultipleEvents_PreservesFifoOrder)
{
    RecordProperty("DerivationTechnique", "requirements-based");
    SupervisionEventQueue queue{8};
    for (std::int64_t i = 0; i < 8; ++i)
    {
        ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, i)));
    }
    for (std::int64_t i = 0; i < 8; ++i)
    {
        SupervisionEvent out{};
        ASSERT_TRUE(queue.tryDequeue(out));
        EXPECT_EQ(out.systemClockTimestamp.tv_nsec, i);
    }
}

TEST_F(SupervisionEventQueue_UT, Overflow_DropsNewest_RejectedEventIsNotConsumed)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    SupervisionEventQueue queue{2};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, 0)));
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, 1)));
    EXPECT_FALSE(queue.tryEnqueue(makeEvent(0, 2)));

    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_EQ(out.systemClockTimestamp.tv_nsec, 0);
    ASSERT_TRUE(queue.tryDequeue(out));
    EXPECT_EQ(out.systemClockTimestamp.tv_nsec, 1);
    EXPECT_FALSE(queue.tryDequeue(out));
}

TEST_F(SupervisionEventQueue_UT, OverflowFlag_IsSticky_AndResettable)
{
    RecordProperty("DerivationTechnique", "requirements-based");
    SupervisionEventQueue queue{1};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, 0)));
    ASSERT_FALSE(queue.tryEnqueue(makeEvent(0, 1)));

    SupervisionEvent out{};
    ASSERT_TRUE(queue.tryDequeue(out));
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, 2)));

    EXPECT_TRUE(queue.getOverflowFlag());
    EXPECT_TRUE(queue.getOverflowFlag(false));
    EXPECT_TRUE(queue.getOverflowFlag(true));
    EXPECT_FALSE(queue.getOverflowFlag());
    EXPECT_FALSE(queue.getOverflowFlag(true));
}

TEST_F(SupervisionEventQueue_UT, ResultMapping_Success_ReturnsTrue_FlagUnchanged)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    std::atomic<bool> flag{false};
    EXPECT_TRUE(SupervisionEventQueueTestPeer::map(score::Result<void>{}, flag));
    EXPECT_FALSE(flag.load());
}

TEST_F(SupervisionEventQueue_UT, ResultMapping_Overflow_ReturnsFalse_SetsFlag)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    std::atomic<bool> flag{false};
    EXPECT_FALSE(
        SupervisionEventQueueTestPeer::map(
            score::Result<void>{score::MakeUnexpected(ConcurrencyErrc::kOverflow)}, flag));
    EXPECT_TRUE(flag.load());
}

TEST_F(SupervisionEventQueue_UT, ResultMapping_Stopped_ReturnsFalse_FlagUnchanged)
{
    RecordProperty("DerivationTechnique", "error-condition-injection");
    std::atomic<bool> flag{false};
    EXPECT_FALSE(
        SupervisionEventQueueTestPeer::map(
            score::Result<void>{score::MakeUnexpected(ConcurrencyErrc::kStopped)}, flag));
    EXPECT_FALSE(flag.load());
}

TEST_F(SupervisionEventQueue_UT, Destructor_WithoutExplicitStop_Succeeds)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    auto queue = std::make_unique<SupervisionEventQueue>(4);
    ASSERT_TRUE(queue->tryEnqueue(makeEvent(0, 0)));
    EXPECT_NO_THROW(queue.reset());
}

// ---------------------------------------------------------------------------------------------
// Stress tests: 16 producers, one dedicated consumer thread (also performs the final drain).
// ---------------------------------------------------------------------------------------------
namespace
{

constexpr std::size_t kProducers = 16;

struct StressResult
{
    std::vector<std::vector<std::int64_t>> successes{kProducers};
    std::vector<std::uint64_t> rejected = std::vector<std::uint64_t>(kProducers, 0);
    std::vector<SupervisionEvent> consumed;
};

void producerLoop(
    SupervisionEventQueue& queue,
    std::size_t producer,
    std::int64_t events,
    std::atomic<bool>& go,
    std::vector<std::int64_t>& successes,
    std::uint64_t& rejected)
{
    while (!go.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    for (std::int64_t seq = 0; seq < events; ++seq)
    {
        if (queue.tryEnqueue(makeEvent(static_cast<std::int64_t>(producer), seq)))
        {
            successes.push_back(seq);
        }
        else
        {
            ++rejected;
        }
    }
}

// consumeDuringProduction: consumer runs concurrently; otherwise it starts after producers finish.
StressResult runStress(std::size_t capacity, std::int64_t eventsPerProducer, bool consumeDuringProduction)
{
    SupervisionEventQueue queue{capacity};
    StressResult result;
    std::atomic<bool> go{false};
    std::atomic<bool> producersDone{false};
    std::atomic<bool> consumerStart{consumeDuringProduction};

    std::thread consumer{[&] {
        while (!consumerStart.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        SupervisionEvent e{};
        for (;;)
        {
            const bool done = producersDone.load(std::memory_order_acquire);
            while (queue.tryDequeue(e))
            {
                result.consumed.push_back(e);
            }
            if (done)
            {
                break;
            }
            std::this_thread::yield();
        }
    }};

    std::vector<std::thread> producers;
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        producers.emplace_back(
            producerLoop,
            std::ref(queue),
            p,
            eventsPerProducer,
            std::ref(go),
            std::ref(result.successes[p]),
            std::ref(result.rejected[p]));
    }
    go.store(true, std::memory_order_release);
    for (auto& t : producers)
    {
        t.join();
    }
    producersDone.store(true, std::memory_order_release);
    consumerStart.store(true, std::memory_order_release);
    consumer.join();

    const bool flag = queue.getOverflowFlag();
    std::uint64_t totalRejected = 0;
    for (auto r : result.rejected)
    {
        totalRejected += r;
    }
    EXPECT_EQ(flag, totalRejected > 0) << "sticky overflow flag must be set iff an overflow occurred";
    return result;
}

void verifyConsumed(const StressResult& r, std::int64_t eventsPerProducer)
{
    std::uint64_t successful = 0;
    std::uint64_t rejected = 0;
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        successful += r.successes[p].size();
        rejected += r.rejected[p];
    }
    EXPECT_EQ(successful + rejected, kProducers * static_cast<std::uint64_t>(eventsPerProducer));
    EXPECT_EQ(r.consumed.size(), successful);

    std::vector<std::vector<std::int64_t>> perProducer(kProducers);
    for (const auto& e : r.consumed)
    {
        ASSERT_TRUE(payloadIntact(e));
        const auto producer = static_cast<std::size_t>(e.systemClockTimestamp.tv_sec);
        ASSERT_LT(producer, kProducers);
        perProducer[producer].push_back(e.systemClockTimestamp.tv_nsec);
    }
    // Per-producer pushes are sequential, so consumed order must equal that producer's success list
    // (covers exactly-once, no duplicates, no rejected events, and per-producer ordering).
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        EXPECT_EQ(perProducer[p], r.successes[p]) << "producer " << p;
    }
}

class SupervisionEventQueue_Stress : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "stress-test");
    }
};

}  // namespace

TEST_F(SupervisionEventQueue_Stress, T1_NoOverflow)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    constexpr std::int64_t kEvents = 200;
    const auto r = runStress(kProducers * kEvents, kEvents, true);
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        EXPECT_EQ(r.rejected[p], 0U);
        // No gaps: every submission succeeded in order.
        ASSERT_EQ(r.successes[p].size(), static_cast<std::size_t>(kEvents));
    }
    verifyConsumed(r, kEvents);
}

TEST_F(SupervisionEventQueue_Stress, T2_OverflowUnderContention)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    constexpr std::int64_t kEvents = 2000;
    const auto r = runStress(8, kEvents, true);
    verifyConsumed(r, kEvents);
}

TEST_F(SupervisionEventQueue_Stress, T3_DeterministicOverflow)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    constexpr std::size_t kCapacity = 64;
    constexpr std::int64_t kEvents = 8;  // 16 * 8 = 128 > capacity
    const auto r = runStress(kCapacity, kEvents, false);

    std::uint64_t successful = 0;
    std::uint64_t rejected = 0;
    for (std::size_t p = 0; p < kProducers; ++p)
    {
        successful += r.successes[p].size();
        rejected += r.rejected[p];
    }
    EXPECT_EQ(successful, kCapacity);
    EXPECT_EQ(rejected, kProducers * kEvents - kCapacity);
    verifyConsumed(r, kEvents);
}

TEST_F(SupervisionEventQueue_Stress, OverflowFlag_ConcurrentReset_NoLostSet)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    SupervisionEventQueue queue{1};
    ASSERT_TRUE(queue.tryEnqueue(makeEvent(0, 0)));

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
                (void)queue.tryEnqueue(makeEvent(0, 1));  // always full: every call overflows
            }
        });
    }
    for (auto& t : producers)
    {
        t.join();
    }
    stop.store(true, std::memory_order_release);
    resetter.join();

    // Overflows occurred after the last reset only if the final flag is set; otherwise a reset consumed it.
    EXPECT_TRUE(queue.getOverflowFlag() || resetsSeenTrue.load() > 0);

    SupervisionEvent out{};
    EXPECT_TRUE(queue.tryDequeue(out));
}

/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/common/constants.hpp"

using namespace testing;
using namespace score::lcm::internal;

namespace
{

constexpr uint32_t kCapacity = static_cast<uint32_t>(ProcessLimits::kMaxProcesses);

class MockTerminationCallback : public ITerminationCallback
{
   public:
    MOCK_METHOD(void, terminated, (int32_t process_status), (override));
};

class SafeProcessMapTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    SafeProcessMap sut_{kCapacity};
    MockTerminationCallback callback_;
};

// --- Construction ---

TEST_F(SafeProcessMapTest, ConstructWithZeroCapacity)
{
    RecordProperty("Description", "SafeProcessMap can be constructed with zero capacity.");

    // when
    SafeProcessMap map(0);
}

// --- findTerminated ---

TEST_F(SafeProcessMapTest, FindTerminatedWithNegativePidReturnsInvalid)
{
    RecordProperty("Description", "findTerminated returns -2 for a negative process ID.");

    // when
    int32_t result = sut_.findTerminated(-1, 1000);

    // then
    EXPECT_EQ(result, -2);
}

TEST_F(SafeProcessMapTest, FindTerminatedInsertsEntryWhenPidNotPresent)
{
    RecordProperty("Description", "findTerminated inserts an entry and returns 1 when the PID is not in the map.");

    // when
    int32_t result = sut_.findTerminated(1000, 0);

    // then
    EXPECT_EQ(result, 1);
}

TEST_F(SafeProcessMapTest, FindTerminatedMatchesExistingInsertAndCallsCallback)
{
    RecordProperty("Description",
                   "findTerminated matches an existing insert entry and invokes the termination callback.");

    // given
    sut_.insertIfNotTerminated(1000, &callback_);

    // then
    EXPECT_CALL(callback_, terminated(42));

    // when
    int32_t result = sut_.findTerminated(1000, 42);
    EXPECT_EQ(result, 0);
}

// --- insertIfNotTerminated ---

TEST_F(SafeProcessMapTest, InsertIntoEmptyTreeReturnsZero)
{
    RecordProperty("Description", "insertIfNotTerminated returns 0 when inserting into an empty tree.");

    // when
    int32_t result = sut_.insertIfNotTerminated(2000, &callback_);

    // then
    EXPECT_EQ(result, 0);
}

TEST_F(SafeProcessMapTest, InsertMatchesExistingFindTerminatedEntry)
{
    RecordProperty("Description",
                   "insertIfNotTerminated returns 1 when matching an entry previously added by findTerminated.");

    // given
    sut_.findTerminated(1000, 0);

    EXPECT_CALL(callback_, terminated(0));

    // when
    int32_t result = sut_.insertIfNotTerminated(1000, &callback_);

    // then
    EXPECT_EQ(result, 1);
}

TEST_F(SafeProcessMapTest, InsertMultipleNodesThenFindTerminatedRemovesAll)
{
    RecordProperty("Description",
                   "Inserting kMaxProcesses nodes and then calling findTerminated for each returns 0.");

    // given
    NiceMock<MockTerminationCallback> callbacks[kCapacity];
    for (uint32_t i = 1; i <= kCapacity; ++i)
    {
        sut_.insertIfNotTerminated(static_cast<int32_t>(i), &callbacks[i - 1]);
    }

    // when / then
    for (uint32_t j = 1; j <= kCapacity; ++j)
    {
        EXPECT_EQ(sut_.findTerminated(static_cast<int32_t>(j), 0), 0);
    }
}

TEST_F(SafeProcessMapTest, InsertBeyondCapacityReturnsOutOfMemory)
{
    RecordProperty("Description",
                   "insertIfNotTerminated returns -1 when the map is full and a new entry is attempted.");

    // given
    NiceMock<MockTerminationCallback> callbacks[kCapacity];
    for (uint32_t i = 0; i < kCapacity; ++i)
    {
        EXPECT_EQ(sut_.insertIfNotTerminated(static_cast<int32_t>(i), &callbacks[i]), 0);
    }

    // when
    int32_t result = sut_.insertIfNotTerminated(static_cast<int32_t>(kCapacity + 1), &callback_);

    // then
    EXPECT_EQ(result, -1);
}

// --- Anomalous (PID reuse) cases ---

TEST_F(SafeProcessMapTest, InsertSamePidTwiceYieldsUntilFindTerminatedResolves)
{
    RecordProperty("Description",
                   "Inserting the same PID twice causes the second insert to yield until findTerminated resolves it.");

    // given
    std::atomic_bool first_done{false};
    int32_t ret1 = 2;
    int32_t ret2 = 2;

    NiceMock<MockTerminationCallback> cb;

    std::thread inserter([&]() {
        ret1 = sut_.insertIfNotTerminated(42, &cb);
        first_done.store(true);
        ret2 = sut_.insertIfNotTerminated(42, &cb);
    });

    // when — wait for first insert to complete
    while (!first_done)
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // then — first succeeded, second is still blocked
    EXPECT_EQ(ret1, 0);
    EXPECT_EQ(ret2, 2);

    // when — resolve the anomaly
    EXPECT_EQ(sut_.findTerminated(42, 0), 0);
    inserter.join();

    // then
    EXPECT_EQ(ret2, 0);
}

TEST_F(SafeProcessMapTest, FindTerminatedSamePidTwiceYieldsUntilInsertResolves)
{
    RecordProperty("Description",
                   "Calling findTerminated twice with the same PID causes the second call to yield "
                   "until insertIfNotTerminated resolves it.");

    // given
    std::atomic_bool first_done{false};
    int32_t ret1 = 2;
    int32_t ret2 = 2;

    NiceMock<MockTerminationCallback> cb;

    std::thread finder([&]() {
        ret1 = sut_.findTerminated(42, 0);
        first_done.store(true);
        ret2 = sut_.findTerminated(42, 0);
    });

    // when — wait for first find to complete
    while (!first_done)
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // then — first succeeded, second is still blocked
    EXPECT_EQ(ret1, 1);
    EXPECT_EQ(ret2, 2);

    // when — resolve the anomaly
    EXPECT_EQ(sut_.insertIfNotTerminated(42, &cb), 1);
    finder.join();

    // then
    EXPECT_EQ(ret2, 1);
}

// --- Max depth tree ---

TEST_F(SafeProcessMapTest, FindTerminatedWorksAtMaxTreeDepth)
{
    RecordProperty("Description", "The binary tree handles maximum depth correctly.");

    // given — build a deep tree using bit patterns that always branch one way
    EXPECT_EQ(sut_.findTerminated(0x00000000, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00000001, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00000002, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00000003, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00000007, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000000F, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000001F, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000003F, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000007F, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x000000FF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x000001FF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x000003FF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x000007FF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00000FFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00001FFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00003FFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00007FFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000FFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0000FFFE, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0001FFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0003FFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0007FFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x000FFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x001FFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x003FFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x007FFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x00FFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x01FFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x03FFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x07FFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x0FFFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x1FFFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x3FFFFFFF, 0), 1);
    EXPECT_EQ(sut_.findTerminated(0x7FFFFFFF, 0), 1);

    // when / then — boundary values
    EXPECT_EQ(sut_.findTerminated(static_cast<int32_t>(0xFFFFFFFF), 0), -2);
    EXPECT_EQ(sut_.insertIfNotTerminated(static_cast<int32_t>(0xFFFFFFFF), &callback_), -2);

    // when / then — retrieve entries using insertIfNotTerminated
    NiceMock<MockTerminationCallback> cb;
    EXPECT_EQ(sut_.insertIfNotTerminated(0x0000FFFE, &cb), 1);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x00010000, &cb), 0);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x0001FFFF, &cb), 1);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x00000002, &cb), 1);
}

// --- Multi-threaded stress tests ---

TEST_F(SafeProcessMapTest, ConcurrentInsertAndFindFromMultipleThreads)
{
    RecordProperty("Description",
                   "Multiple threads concurrently inserting and finding terminated processes completes without error.");

    // given
    constexpr int kNumThreads = 4;
    constexpr int kIterations = 1000;
    constexpr int kPidsPerThread = 256;
    NiceMock<MockTerminationCallback> stubs[kNumThreads];
    int results[kNumThreads] = {};

    // when
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            int base = 1000 + t * kPidsPerThread;
            for (int j = 0; j < kIterations; ++j)
            {
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    results[t] = sut_.insertIfNotTerminated(base + i, &stubs[t]);
                }
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    sut_.findTerminated(base + i, 0);
                }
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // then
    for (int t = 0; t < kNumThreads; ++t)
    {
        EXPECT_EQ(results[t], 0);
    }
}

TEST_F(SafeProcessMapTest, ConcurrentFindAndInsertFromMultipleThreads)
{
    RecordProperty("Description",
                   "Multiple threads concurrently finding and inserting processes completes without error.");

    // given
    constexpr int kNumThreads = 4;
    constexpr int kIterations = 1000;
    constexpr int kPidsPerThread = 256;
    NiceMock<MockTerminationCallback> stubs[kNumThreads];
    int results[kNumThreads] = {};

    // when
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            int base = 1000 + t * kPidsPerThread;
            for (int j = 0; j < kIterations; ++j)
            {
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    results[t] = sut_.findTerminated(base + i, 0);
                }
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    sut_.insertIfNotTerminated(base + i, &stubs[t]);
                }
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // then
    for (int t = 0; t < kNumThreads; ++t)
    {
        EXPECT_EQ(results[t], 1);
    }
}

}  // namespace

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

#include "termination_waiter.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

using namespace score::mw::lifecycle::internal;

namespace
{

using namespace std::chrono_literals;

class TerminationWaiterTest : public ::testing::Test
{
  protected:
    // Helper to provide a default/inactive stop token
    score::cpp::stop_token dummy_token{};
};

TEST_F(TerminationWaiterTest, Given0MillisecondsTimeout_WhenWaitWithTimeoutCalled)
{
    // Lower boundary of the timeout parameter is tested
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a 0ms timeout. "
        "When calling wait_with_timeout on a non-terminated waiter. "
        "Then wait_with_timeout should return false immediately.");

    // Given
    TerminationWaiter waiter;
    constexpr std::chrono::milliseconds ZERO_TIMEOUT{0ms};

    // When
    bool result = waiter.wait_with_timeout(dummy_token, ZERO_TIMEOUT);

    // Then
    EXPECT_FALSE(result);
}

TEST_F(TerminationWaiterTest, Given50MillisecondsTimeout_WhenWaitWithTimeoutCalled)
{
    // All timeout values are in the same equivalence class,
    // where wait_with_timeout will return false after a delay
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a 50ms timeout. "
        "When wait_with_timeout is called a non-terminated waiter. "
        "Then wait_with_timeout should return after at least 50ms.");

    // Given
    TerminationWaiter waiter;
    constexpr std::chrono::milliseconds TIMEOUT{50ms};

    // When
    const auto start = std::chrono::steady_clock::now();
    bool result = waiter.wait_with_timeout(dummy_token, TIMEOUT);
    const auto end = std::chrono::steady_clock::now();

    // Then
    EXPECT_FALSE(result);
    EXPECT_GE(end - start, TIMEOUT);
}

TEST_F(TerminationWaiterTest, GivenAnyTimeoutAndATerminatedWaiter_WhenWaitWithTimeoutCalled)
{
    // All timeout values are in the same equivalence class,
    // where wait_with_timeout will return immediately
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given any timeout value and a TerminationWaiter that is already terminated. "
        "When calling wait_with_timeout. "
        "Then wait_with_timeout unblocks immediately and returns true.");

    // Given
    TerminationWaiter waiter;
    waiter.terminated();
    constexpr std::chrono::milliseconds TIMEOUT{1000ms};

    // When
    const auto start = std::chrono::steady_clock::now();
    bool result = waiter.wait_with_timeout(dummy_token, TIMEOUT);
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    // Then
    EXPECT_TRUE(result);
    EXPECT_LT(duration, TIMEOUT);
}

TEST_F(TerminationWaiterTest, GivenAWaitingWaiterTerminationWaiter_WhenTerminatedCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a waiting TerminationWaiter. "
        "When calling terminated(). "
        "Then wait_with_timeout() unblocks immediately and returns true.");

    // Given
    TerminationWaiter waiter;
    std::mutex sync_mtx;
    std::condition_variable sync_cv;
    bool thread_is_ready = false;

    auto future = std::async(std::launch::async, [&]() {
        {
            std::lock_guard<std::mutex> lock(sync_mtx);
            thread_is_ready = true;
        }
        sync_cv.notify_one();

        return waiter.wait_with_timeout(dummy_token, 2000ms);
    });

    {
        std::unique_lock<std::mutex> lock(sync_mtx);
        sync_cv.wait(lock, [&] {
            return thread_is_ready;
        });
    }

    // When
    waiter.terminated();

    // Then
    ASSERT_EQ(future.wait_for(500ms), std::future_status::ready);
    EXPECT_TRUE(future.get());
}

TEST_F(TerminationWaiterTest, GivenMultipleWaitingThreads_WhenTerminatedCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given multiple threads waiting with wait_with_timeout. "
        "When calling terminated(). "
        "Then all threads unblock on wait_with_timeout() and return true.");

    // Given
    TerminationWaiter waiter;
    constexpr std::size_t kNumWaiters = 5;
    std::mutex sync_mtx;
    std::condition_variable sync_cv;
    std::size_t ready_count = 0;

    std::vector<std::future<bool>> futures;
    futures.reserve(kNumWaiters);

    for (std::size_t i = 0; i < kNumWaiters; ++i)
    {
        futures.push_back(std::async(std::launch::async, [&]() {
            {
                std::lock_guard<std::mutex> lock(sync_mtx);
                ++ready_count;
            }
            sync_cv.notify_one();

            return waiter.wait_with_timeout(dummy_token, 3000ms);
        }));
    }

    {
        std::unique_lock<std::mutex> lock(sync_mtx);
        sync_cv.wait(lock, [&] {
            return ready_count == kNumWaiters;
        });
    }

    // When
    waiter.terminated();

    // Then
    for (auto& fut : futures)
    {
        ASSERT_EQ(fut.wait_for(500ms), std::future_status::ready);
        EXPECT_TRUE(fut.get());
    }
}

TEST_F(TerminationWaiterTest, GivenAStoppedStopToken_WhenWaitWithTimeoutCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given an already-stopped stop_token. "
        "When calling wait_with_timeout() with a large timeout. "
        "Then wait_with_timeout() exits immediately, returning false.");

    // Given
    TerminationWaiter waiter;
    score::cpp::stop_source stop_source;
    stop_source.request_stop();
    score::cpp::stop_token pre_stopped_token = stop_source.get_token();

    // When
    const auto start = std::chrono::steady_clock::now();
    bool result = waiter.wait_with_timeout(pre_stopped_token, 5000ms);
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    // Then
    EXPECT_FALSE(result);
    EXPECT_LT(duration, 50ms);
}

TEST_F(TerminationWaiterTest, GivenAWaitingWaiter_WhenStoppedByTokenMidWait)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a waiting TerminationWaiter object. "
        "When stopped by token mid-wait. "
        "Then the wait is aborted and returns false.");

    // Given
    TerminationWaiter waiter;
    score::cpp::stop_source stop_source;
    score::cpp::stop_token stop_token = stop_source.get_token();

    std::mutex sync_mtx;
    std::condition_variable sync_cv;
    bool thread_started = false;

    auto future = std::async(std::launch::async, [&]() {
        {
            std::lock_guard<std::mutex> lock(sync_mtx);
            thread_started = true;
        }
        sync_cv.notify_one();

        return waiter.wait_with_timeout(stop_token, 5000ms);
    });

    {
        std::unique_lock<std::mutex> lock(sync_mtx);
        sync_cv.wait(lock, [&] {
            return thread_started;
        });
    }

    // When stopped by token mid-wait
    stop_source.request_stop();

    // Then
    ASSERT_EQ(future.wait_for(500ms), std::future_status::ready);
    EXPECT_FALSE(future.get());
}

TEST_F(TerminationWaiterTest, GivenTerminationWaiter_WhenTerminatedCalledMultipleTimes)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a TerminationWaiter object. "
        "When calling terminated() multiple times. "
        "Then wait_with_timeout() still functions normally and reports true.");

    // Given
    TerminationWaiter waiter;

    // When
    EXPECT_NO_THROW(waiter.terminated());
    EXPECT_NO_THROW(waiter.terminated());
    EXPECT_NO_THROW(waiter.terminated());

    // Then
    EXPECT_TRUE(waiter.wait_with_timeout(dummy_token, 0ms));
}

}  // namespace

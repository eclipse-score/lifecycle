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

#include "score/health_monitor/src/cpp/internal/health_monitor_impl.h"

#include "score/health_monitor/deadline.h"
#include "score/health_monitor/deadline_monitor.h"
#include "score/health_monitor/heartbeat_monitor.h"
#include "score/health_monitor/logic_monitor.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/os/time.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>
#include <utility>

// This binary replaces the global allocation functions, so it counts every `operator new` of every thread.
// It is separate from :tests, because the replacement applies to the whole program.

namespace
{

std::atomic<bool> g_counting{false};
std::atomic<std::size_t> g_allocations{0U};

void* CountedAllocate(const std::size_t size) noexcept
{
    if (g_counting.load(std::memory_order_relaxed))
    {
        g_allocations.fetch_add(1U, std::memory_order_relaxed);
    }
    return std::malloc((size == 0U) ? 1U : size);
}

void* CountedAllocateAligned(const std::size_t size, const std::align_val_t alignment) noexcept
{
    if (g_counting.load(std::memory_order_relaxed))
    {
        g_allocations.fetch_add(1U, std::memory_order_relaxed);
    }
    const auto align = static_cast<std::size_t>(alignment);
    const std::size_t rounded{((size + align - 1U) / align) * align};
    return std::aligned_alloc(align, (rounded == 0U) ? align : rounded);
}

void* AllocateOrAbort(void* const memory) noexcept
{
    if (memory == nullptr)
    {
        std::abort();
    }
    return memory;
}

}  // namespace

void* operator new(const std::size_t size)
{
    return AllocateOrAbort(CountedAllocate(size));
}

void* operator new[](const std::size_t size)
{
    return AllocateOrAbort(CountedAllocate(size));
}

void* operator new(const std::size_t size, const std::nothrow_t&) noexcept
{
    return CountedAllocate(size);
}

void* operator new[](const std::size_t size, const std::nothrow_t&) noexcept
{
    return CountedAllocate(size);
}

void* operator new(const std::size_t size, const std::align_val_t alignment)
{
    return AllocateOrAbort(CountedAllocateAligned(size, alignment));
}

void* operator new[](const std::size_t size, const std::align_val_t alignment)
{
    return AllocateOrAbort(CountedAllocateAligned(size, alignment));
}

void* operator new(const std::size_t size, const std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return CountedAllocateAligned(size, alignment);
}

void* operator new[](const std::size_t size, const std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return CountedAllocateAligned(size, alignment);
}

void operator delete(void* const memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory) noexcept
{
    std::free(memory);
}

void operator delete(void* const memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete(void* const memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete(void* const memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* const memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* const memory, std::align_val_t, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* const memory, std::align_val_t, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

namespace score::health_monitor::details
{
namespace
{

using namespace std::chrono_literals;

constexpr MonitorTag kDeadlineMonitor{"deadline_monitor"};
constexpr MonitorTag kHeartbeatMonitor{"heartbeat_monitor"};
constexpr MonitorTag kLogicMonitor{"logic_monitor"};
constexpr DeadlineTag kDeadline{"deadline"};
constexpr StateTag kStateA{"a"};
constexpr StateTag kStateB{"b"};

/// @brief Supervisor client that counts alive notifications and allocates nothing, unlike a gmock mock.
class CountingSupervisorClient final : public SupervisorClient
{
  public:
    explicit CountingSupervisorClient(std::atomic<std::size_t>& notifications) noexcept : notifications_{notifications}
    {
    }

    score::Result<void> Connect() noexcept override
    {
        return {};
    }

    void NotifyAlive() noexcept override
    {
        notifications_.fetch_add(1U);
    }

  private:
    std::atomic<std::size_t>& notifications_;
};

/// @brief Stops counting allocations when destroyed, so a failed assertion does not leave counting on.
class ScopedAllocationCount final
{
  public:
    ScopedAllocationCount() noexcept
    {
        g_allocations.store(0U);
        g_counting.store(true);
    }

    ScopedAllocationCount(const ScopedAllocationCount&) = delete;
    ScopedAllocationCount(ScopedAllocationCount&&) = delete;
    ScopedAllocationCount& operator=(const ScopedAllocationCount&) = delete;
    ScopedAllocationCount& operator=(ScopedAllocationCount&&) = delete;

    ~ScopedAllocationCount() noexcept
    {
        g_counting.store(false);
    }

    std::size_t Stop() noexcept
    {
        g_counting.store(false);
        return g_allocations.load();
    }
};

TEST(StaticMemoryTest, NoAllocationAfterStart)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__static_memory, comp_req__health_monitor__nonblocking_api");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that no thread allocates through operator new after Start returns, while the application reports "
        "heartbeats, deadline starts and stops and state transitions, and the worker evaluates and notifies alive.");

    HealthMonitorImpl::Config config;
    config.deadline_monitors.push_back(
        HealthMonitorImpl::Config::DeadlineMonitorConfig{
            kDeadlineMonitor, {DeadlineConfig{kDeadline, TimeRange{0ms, 5000ms}}}});
    config.heartbeat_monitors.push_back(
        HealthMonitorImpl::Config::HeartbeatMonitorConfig{kHeartbeatMonitor, TimeRange{10ms, 5000ms}});
    config.logic_monitors.push_back(
        HealthMonitorImpl::Config::LogicMonitorConfig{
            kLogicMonitor, kStateA, {StateNode{kStateA, {kStateB}}, StateNode{kStateB, {kStateA}}}});
    config.internal_processing_cycle = 5ms;
    config.supervisor_api_cycle = 5ms;

    std::atomic<std::size_t> notifications{0U};
    auto created = HealthMonitorImpl::Create(
        std::move(config), std::make_unique<CountingSupervisorClient>(notifications), score::os::Time::instance());
    ASSERT_TRUE(created.has_value());
    const auto health_monitor = std::move(created).value();

    auto deadline_monitor = health_monitor->GetDeadlineMonitor(kDeadlineMonitor);
    auto heartbeat_monitor = health_monitor->GetHeartbeatMonitor(kHeartbeatMonitor);
    auto logic_monitor = health_monitor->GetLogicMonitor(kLogicMonitor);
    ASSERT_TRUE(deadline_monitor.has_value());
    ASSERT_TRUE(heartbeat_monitor.has_value());
    ASSERT_TRUE(logic_monitor.has_value());
    auto deadline = deadline_monitor.value()->GetDeadline(kDeadline);
    ASSERT_TRUE(deadline.has_value());

    ASSERT_TRUE(health_monitor->Start().has_value());

    std::size_t allocations{0U};
    std::size_t notifications_before_last_round{0U};
    {
        ScopedAllocationCount count;
        bool in_state_a{true};
        const auto end = std::chrono::steady_clock::now() + 300ms;
        for (std::size_t round{0U}; std::chrono::steady_clock::now() < end; ++round)
        {
            // One heartbeat per 50 ms, the first about 25 ms after Start, so each is above the 10 ms minimum and no
            // internal processing cycle sees two.
            if ((round % 10U) == 5U)
            {
                heartbeat_monitor.value()->Heartbeat();
            }
            {
                auto handle = deadline.value()->Start();
                EXPECT_TRUE(handle.has_value());
            }
            EXPECT_TRUE(logic_monitor.value()->Transition(in_state_a ? kStateB : kStateA).has_value());
            in_state_a = !in_state_a;
            std::this_thread::sleep_for(5ms);
        }
        notifications_before_last_round = notifications.load();
        // The worker still notifies alive, so every monitor passed its evaluations.
        EXPECT_TRUE(WaitFor([&notifications, notifications_before_last_round] {
            return notifications.load() > notifications_before_last_round;
        }));
        allocations = count.Stop();
    }

    EXPECT_GT(notifications_before_last_round, 10U);
    EXPECT_EQ(allocations, 0U);
}

}  // namespace
}  // namespace score::health_monitor::details

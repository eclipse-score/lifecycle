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

#include <dlfcn.h>
#include <time.h>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;

namespace
{
std::atomic<bool> g_failClockGetRes{false};
// 0: disarmed, 1: next allocation throws std::bad_alloc, 2: next allocation throws a non-std type
// Thread-local so allocations on other threads cannot consume the armed failure.
thread_local std::atomic<int> g_failNextAllocation{0};
}  // namespace

// Overrides libc's clock_getres so tests can make the clock resolution query fail.
extern "C" int clock_getres(clockid_t clock_id, struct timespec* res)
{
    if (g_failClockGetRes.load())
    {
        return -1;
    }
    using Fn = int (*)(clockid_t, struct timespec*);
    static const Fn real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "clock_getres"));
    return real(clock_id, res);
}

void* operator new(std::size_t size)
{
    const int mode = g_failNextAllocation.exchange(0);
    if (mode == 1)
    {
        throw std::bad_alloc{};
    }
    if (mode == 2)
    {
        throw 42;
    }
    void* ptr = std::malloc(size == 0 ? 1 : size);
    if (ptr == nullptr)
    {
        throw std::bad_alloc{};
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

namespace score::mw::lifecycle::internal::saf::daemon
{

class MockRecoveryClient : public IRecoveryClient
{
  public:
    MOCK_METHOD(
        void,
        setRecoveryRequestCallback,
        (IRecoveryClient::RecoveryRequestCallback callback),
        (noexcept, override));
    MOCK_METHOD(bool, sendRecoveryRequest, (const IdentifierHash& process_group_identifier), (noexcept, override));
};

class AliveMonitorImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "boundary-values");
        mockClient_ = std::make_shared<MockRecoveryClient>();
    }

    std::shared_ptr<MockRecoveryClient> mockClient_;
};

// --------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------

TEST_F(AliveMonitorImplTest, Constructor_WithValidParameters_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with valid parameters.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);

    ASSERT_NE(&monitor, nullptr);
}

TEST_F(AliveMonitorImplTest, Constructor_WithMinimalConfig_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with minimal configuration.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 1);

    ASSERT_NE(&monitor, nullptr);
}

TEST_F(AliveMonitorImplTest, Constructor_WithLargeComponentCount_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with large component count.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 100);

    ASSERT_NE(&monitor, nullptr);
}

// --------------------------------------------------------------------------
// init
// --------------------------------------------------------------------------

TEST_F(AliveMonitorImplTest, Init_WithValidConfig_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true with valid configuration.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    bool result = monitor.init();

    ASSERT_TRUE(result);
}

TEST_F(AliveMonitorImplTest, Init_WithZeroCycleTime_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true even with zero cycle time.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 0;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    const bool result = monitor.init();

    // Depending on clock accuracy, this might fail or succeed
    // We're testing it doesn't crash
    ASSERT_TRUE(result);
}

TEST_F(AliveMonitorImplTest, Init_WithLargeCycleTime_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true with large cycle time.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 60000;  // 60 seconds

    AliveMonitorImpl monitor(mockClient_, config, 5);
    bool result = monitor.init();

    ASSERT_TRUE(result);
}

TEST_F(AliveMonitorImplTest, Init_CanBeCalledMultipleTimes)
{
    RecordProperty("Description", "Verify that init can be called multiple times safely.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);

    bool result1 = monitor.init();
    bool result2 = monitor.init();

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
}

// --------------------------------------------------------------------------
// startMonitoring and stopMonitoring
// --------------------------------------------------------------------------

TEST_F(AliveMonitorImplTest, StartMonitoring_CreatesThread)
{
    RecordProperty("Description", "Verify that startMonitoring creates a monitoring thread.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    monitor.startMonitoring();
    // Give thread a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    monitor.stopMonitoring();
}

TEST_F(AliveMonitorImplTest, StopMonitoring_StopsThread)
{
    RecordProperty("Description", "Verify that stopMonitoring stops the monitoring thread.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    monitor.startMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    monitor.stopMonitoring();
    // If we reach here without deadlock, the test passed
}

TEST_F(AliveMonitorImplTest, StartAndStopMonitoring_CanBeCalledMultipleTimes)
{
    RecordProperty(
        "Description",
        "Verify that startMonitoring and stopMonitoring can be called "
        "multiple times without deadlock or exceptions.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    // First cycle
    monitor.startMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    monitor.stopMonitoring();

    // Second cycle
    monitor.startMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    monitor.stopMonitoring();

    SUCCEED();
}

// --------------------------------------------------------------------------
// getSupervisionFactory
// --------------------------------------------------------------------------

TEST_F(AliveMonitorImplTest, GetSupervisionFactory_ReturnsValidReference)
{
    RecordProperty("Description", "Verify that getSupervisionFactory returns a valid factory reference.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    ISupervisionFactory& factory = monitor.getSupervisionFactory();
    ASSERT_NE(&factory, nullptr);
}

TEST_F(AliveMonitorImplTest, GetSupervisionFactory_ReturnsConsistentReference)
{
    RecordProperty("Description", "Verify that getSupervisionFactory returns the same reference on multiple calls.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    ISupervisionFactory& factory1 = monitor.getSupervisionFactory();
    ISupervisionFactory& factory2 = monitor.getSupervisionFactory();

    ASSERT_EQ(&factory1, &factory2);
}

// --------------------------------------------------------------------------
// init error paths
// --------------------------------------------------------------------------

TEST_F(AliveMonitorImplTest, Init_ClockResolutionUnavailable_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_failClockGetRes.store(true);
    const bool result = monitor.init();
    g_failClockGetRes.store(false);

    ASSERT_FALSE(result);
}

TEST_F(AliveMonitorImplTest, Init_StdExceptionDuringInit_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_failNextAllocation.store(1);
    const bool result = monitor.init();
    const bool failureConsumed = (g_failNextAllocation.load() == 0);
    g_failNextAllocation.store(0);

    ASSERT_TRUE(failureConsumed);
    ASSERT_FALSE(result);
}

TEST_F(AliveMonitorImplTest, Init_NonStdExceptionDuringInit_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_failNextAllocation.store(2);
    const bool result = monitor.init();
    const bool failureConsumed = (g_failNextAllocation.load() == 0);
    g_failNextAllocation.store(0);

    ASSERT_TRUE(failureConsumed);
    ASSERT_FALSE(result);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon

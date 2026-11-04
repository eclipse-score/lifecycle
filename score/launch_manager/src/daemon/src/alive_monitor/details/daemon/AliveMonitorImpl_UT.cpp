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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"
#include "tests/utils/mocks/mock_syscalls.hpp"

using namespace testing;

namespace
{
// 0: disarmed, 1: next allocation throws std::bad_alloc, 2: next allocation throws a non-std type
// Thread-local so allocations on other threads cannot consume the armed failure.
thread_local std::atomic<int> g_failNextAllocation{0};
}  // namespace

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
    MOCK_METHOD(
        bool,
        sendRecoveryRequest,
        (const IdentifierHash& process_group_identifier, std::chrono::nanoseconds timeout),
        (noexcept, override));
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

class AliveMonitorImplConstructorTest : public AliveMonitorImplTest
{
};

TEST_F(AliveMonitorImplConstructorTest, Constructor_WithValidParameters_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with valid parameters.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);

    ASSERT_NE(&monitor, nullptr);
}

TEST_F(AliveMonitorImplConstructorTest, Constructor_WithMinimalConfig_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with minimal configuration.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 1);

    ASSERT_NE(&monitor, nullptr);
}

TEST_F(AliveMonitorImplConstructorTest, Constructor_WithLargeComponentCount_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that AliveMonitorImpl can be constructed with large component count.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 100);

    ASSERT_NE(&monitor, nullptr);
}

class AliveMonitorImplInitTest : public AliveMonitorImplTest
{
};

TEST_F(AliveMonitorImplInitTest, Init_WithValidConfig_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true with valid configuration.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    bool result = monitor.init();

    EXPECT_TRUE(result);
}

TEST_F(AliveMonitorImplInitTest, Init_WithZeroCycleTime_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true even with zero cycle time.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 0;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    const bool result = monitor.init();

    // Depending on clock accuracy, this might fail or succeed
    // We're testing it doesn't crash
    EXPECT_TRUE(result);
}

TEST_F(AliveMonitorImplInitTest, Init_WithLargeCycleTime_ReturnsTrue)
{
    RecordProperty("Description", "Verify that init returns true with large cycle time.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 60000;  // 60 seconds

    AliveMonitorImpl monitor(mockClient_, config, 5);
    bool result = monitor.init();

    EXPECT_TRUE(result);
}

TEST_F(AliveMonitorImplInitTest, Init_CanBeCalledMultipleTimes)
{
    RecordProperty("Description", "Verify that init can be called multiple times safely.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);

    bool result1 = monitor.init();
    bool result2 = monitor.init();

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
}

class AliveMonitorImplGetSupervisionFactoryTest : public AliveMonitorImplTest
{
};

TEST_F(AliveMonitorImplGetSupervisionFactoryTest, GetSupervisionFactory_ReturnsValidReference)
{
    RecordProperty("Description", "Verify that getSupervisionFactory returns a valid factory reference.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    ISupervisionFactory& factory = monitor.getSupervisionFactory();
    ASSERT_NE(&factory, nullptr);
}

class AliveMonitorImplMonitoringTest : public AliveMonitorImplTest
{
};

TEST_F(AliveMonitorImplMonitoringTest, StartAndStopMonitoring_AfterInit_ReturnsWithoutDeadlock)
{
    RecordProperty("Description", "Verify that the monitoring thread can be started and stopped after init.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    monitor.startMonitoring();
    monitor.stopMonitoring();

    SUCCEED();
}

TEST_F(AliveMonitorImplMonitoringTest, StopMonitoring_WithoutStart_ReturnsWithoutDeadlock)
{
    RecordProperty("Description", "Verify that stopMonitoring is safe when no thread was started.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    monitor.stopMonitoring();

    SUCCEED();
}

TEST_F(AliveMonitorImplMonitoringTest, StartAndStopMonitoring_CalledTwice_ReturnsWithoutDeadlock)
{
    RecordProperty("Description", "Verify that start/stop can be repeated without deadlock or exceptions.");

    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 50;

    AliveMonitorImpl monitor(mockClient_, config, 5);
    ASSERT_TRUE(monitor.init());

    monitor.startMonitoring();
    monitor.stopMonitoring();
    monitor.startMonitoring();
    monitor.stopMonitoring();

    SUCCEED();
}

class AliveMonitorImplInitErrorPathsTest : public AliveMonitorImplTest
{
};

TEST_F(AliveMonitorImplInitErrorPathsTest, Init_ClockResolutionUnavailable_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_syscall_mock = std::make_unique<NiceMock<SyscallMock>>();
    ON_CALL(*g_syscall_mock, clock_getres).WillByDefault(Invoke(__real_clock_getres));
    ON_CALL(*g_syscall_mock, clock_getres(CLOCK_MONOTONIC, _)).WillByDefault(Return(-1));
    const bool result = monitor.init();
    g_syscall_mock.reset();

    EXPECT_FALSE(result);
}

TEST_F(AliveMonitorImplInitErrorPathsTest, Init_StdExceptionDuringInit_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_failNextAllocation.store(1);
    const bool result = monitor.init();
    const bool failureConsumed = (g_failNextAllocation.load() == 0);
    g_failNextAllocation.store(0);

    EXPECT_TRUE(failureConsumed);
    EXPECT_FALSE(result);
}

TEST_F(AliveMonitorImplInitErrorPathsTest, Init_NonStdExceptionDuringInit_ReturnsFalse)
{
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;
    AliveMonitorImpl monitor(mockClient_, config, 5);

    g_failNextAllocation.store(2);
    const bool result = monitor.init();
    const bool failureConsumed = (g_failNextAllocation.load() == 0);
    g_failNextAllocation.store(0);

    EXPECT_TRUE(failureConsumed);
    EXPECT_FALSE(result);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon

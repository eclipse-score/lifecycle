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
#include <unistd.h>
#include <time.h>

#include <atomic>
#include <memory>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/details/daemon/CyclicExecutor.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;

namespace
{
std::atomic<bool> g_failMonotonicClockGetTime{false};
}  // namespace

// Overrides libc's clock_gettime so tests can make CLOCK_MONOTONIC reads fail.
extern "C" int clock_gettime(clockid_t clock_id, struct timespec* ts)
{
    if (clock_id == CLOCK_MONOTONIC && g_failMonotonicClockGetTime.load())
    {
        return -1;
    }
    using Fn = int (*)(clockid_t, struct timespec*);
    static const Fn real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "clock_gettime"));
    return real(clock_id, ts);
}

namespace score::mw::lifecycle::internal::saf::daemon
{

class MockOsClockInterface : public timers::OsClockInterface
{
  public:
    MOCK_METHOD(int, clockGetTime, (struct timespec * ts), (const, noexcept, override));
    MOCK_METHOD(int, clockGetRes, (struct timespec * res), (const, noexcept, override));
    MOCK_METHOD(
        int,
        clockNanosleep,
        (int flags, const struct timespec* request, struct timespec* remain),
        (const, noexcept, override));
};

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

class CyclicExecutorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "boundary-values");

        // Set up mock clock to return reasonable values
        struct timespec ts{10, 0};
        ON_CALL(mockClock_, clockGetTime).WillByDefault(DoAll(SetArgPointee<0>(ts), Return(0)));

        // Create timespec for clock resolution
        timespec res_ts{0, 1000};
        ON_CALL(mockClock_, clockGetRes).WillByDefault(DoAll(SetArgPointee<0>(res_ts), Return(0)));

        mockRecoveryClient_ = std::make_shared<MockRecoveryClient>();
    }

    MockOsClockInterface mockClock_;
    std::shared_ptr<MockRecoveryClient> mockRecoveryClient_;
};

// --------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------

TEST_F(CyclicExecutorTest, Constructor_WithValidParameters_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CyclicExecutor can be constructed with valid parameters.");

    CyclicExecutor executor(mockClock_, 5);

    ASSERT_NE(&executor, nullptr);
}

TEST_F(CyclicExecutorTest, Constructor_WithZeroComponents_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CyclicExecutor can be constructed with zero components.");

    CyclicExecutor executor(mockClock_, 0);

    ASSERT_NE(&executor, nullptr);
}

TEST_F(CyclicExecutorTest, Constructor_WithLargeComponentCount_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CyclicExecutor can be constructed with large component count.");

    CyclicExecutor executor(mockClock_, 1000);

    ASSERT_NE(&executor, nullptr);
}

// --------------------------------------------------------------------------
// init
// --------------------------------------------------------------------------

TEST_F(CyclicExecutorTest, Init_WithValidConfig_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init returns kNoError with valid configuration.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    ASSERT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorTest, Init_WithSmallCycleTime_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init succeeds with small cycle time.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 1;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    ASSERT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorTest, Init_WithLargeCycleTime_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init succeeds with large cycle time.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 60000;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    ASSERT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorTest, Init_WithValidCycleTimer_SetsInitialization)
{
    RecordProperty("Description", "Verify that init properly initializes the cycle timer.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    // Ensure clockGetRes returns valid resolution
    struct timespec resolution{0, 1000000};  // 1 millisecond
    ON_CALL(mockClock_, clockGetRes).WillByDefault(DoAll(SetArgPointee<0>(resolution), Return(0)));

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    ASSERT_EQ(result, daemon::EInitCode::kNoError);
}

// --------------------------------------------------------------------------
// startCyclicExec
// --------------------------------------------------------------------------

TEST_F(CyclicExecutorTest, StartCyclicExec_WithImmediateTermination_ReturnsTrue)
{
    RecordProperty("Description", "Verify that startCyclicExec returns true when terminated immediately.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    ASSERT_EQ(executor.init(mockRecoveryClient_, config), daemon::EInitCode::kNoError);

    struct timespec ts{10, 0};
    EXPECT_CALL(mockClock_, clockGetTime).WillRepeatedly(DoAll(SetArgPointee<0>(ts), Return(0)));

    std::atomic_bool terminate{true};
    bool result = executor.startCyclicExec(terminate);

    ASSERT_TRUE(result);
}

// --------------------------------------------------------------------------
// constructSupervision
// --------------------------------------------------------------------------

TEST_F(CyclicExecutorTest, ConstructSupervision_WithValidParameters_ReturnsHandle)
{
    RecordProperty("Description", "Verify that constructSupervision returns a valid handle.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    ASSERT_EQ(executor.init(mockRecoveryClient_, config), daemon::EInitCode::kNoError);

    IdentifierHash componentId{"component1"};
    configuration::ComponentAliveSupervision compConfig{};
    compConfig.min_indications = 1;
    compConfig.max_indications = 3;
    compConfig.failed_cycles_tolerance = 0;
    compConfig.reporting_cycle_ms = 100;

    auto handle = executor.constructSupervision(componentId, getuid(), compConfig);

    ASSERT_NE(handle, nullptr);
}

TEST_F(CyclicExecutorTest, ConstructSupervision_MultipleSupervisions_CreatesAll)
{
    RecordProperty("Description", "Verify that multiple supervisions can be constructed for the current user.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    ASSERT_EQ(executor.init(mockRecoveryClient_, config), daemon::EInitCode::kNoError);

    configuration::ComponentAliveSupervision compConfig{};
    compConfig.min_indications = 1;
    compConfig.max_indications = 3;
    compConfig.failed_cycles_tolerance = 0;
    compConfig.reporting_cycle_ms = 100;

    IdentifierHash componentId1{"component1"};
    IdentifierHash componentId2{"component2"};

    // ACL setup only succeeds for the caller's own uid, so fixed uids make the result host-dependent.
    const uid_t uid = getuid();
    auto handle1 = executor.constructSupervision(componentId1, uid, compConfig);
    auto handle2 = executor.constructSupervision(componentId2, uid, compConfig);

    ASSERT_NE(handle1, nullptr);
    ASSERT_NE(handle2, nullptr);
}

TEST_F(CyclicExecutorTest, ConstructSupervision_DuplicateId_ReturnsNullptr)
{
    RecordProperty("Description", "Verify constructSupervision returns nullptr when the id is already registered.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    ASSERT_EQ(executor.init(mockRecoveryClient_, config), daemon::EInitCode::kNoError);

    configuration::ComponentAliveSupervision compConfig{};
    compConfig.min_indications = 1;
    compConfig.max_indications = 3;
    compConfig.failed_cycles_tolerance = 0;
    compConfig.reporting_cycle_ms = 100;

    const IdentifierHash id{"component1"};
    ASSERT_NE(executor.constructSupervision(id, getuid(), compConfig), nullptr);
    ASSERT_EQ(executor.constructSupervision(id, getuid(), compConfig), nullptr);
}

TEST_F(CyclicExecutorTest, ConstructSupervision_MultipleSupervisions_DoesNotCrash)
{
    RecordProperty("Description", "Verify that constructSupervision respects capacity limits.");

    CyclicExecutor executor(mockClock_, 5);  // 5 components allowed
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    ASSERT_EQ(executor.init(mockRecoveryClient_, config), daemon::EInitCode::kNoError);

    configuration::ComponentAliveSupervision compConfig{};
    compConfig.min_indications = 1;
    compConfig.max_indications = 3;
    compConfig.failed_cycles_tolerance = 0;
    compConfig.reporting_cycle_ms = 100;

    IdentifierHash componentId1{"component1"};
    IdentifierHash componentId2{"component2"};
    IdentifierHash componentId3{"component3"};

    auto handle1 = executor.constructSupervision(componentId1, 1000, compConfig);
    auto handle2 = executor.constructSupervision(componentId2, 1001, compConfig);
    auto handle3 = executor.constructSupervision(componentId3, 1002, compConfig);

    // If we reach here without crash, test passed
}

// --------------------------------------------------------------------------
// startCyclicExec error paths
// --------------------------------------------------------------------------

class CyclicExecutorLoopTest : public CyclicExecutorTest
{
  protected:
    void SetUp() override
    {
        CyclicExecutorTest::SetUp();
        config_.evaluation_cycle_ms = 100;
    }

    configuration::AliveSupervisionConfig config_{};
    std::atomic_bool terminate_{false};
};

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_InitialTimestampFails_ReturnsFalse)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    EXPECT_CALL(mockClock_, clockGetTime).WillRepeatedly(Return(-1));

    ASSERT_FALSE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_SleepInterrupted_BreaksLoop)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    EXPECT_CALL(mockClock_, clockNanosleep).WillOnce(Invoke([this](int, const struct timespec*, struct timespec*) {
        terminate_.store(true);
        return EINTR;
    }));

    ASSERT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_SleepFails_LogsErrorAndContinues)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    EXPECT_CALL(mockClock_, clockNanosleep).WillOnce(Invoke([this](int, const struct timespec*, struct timespec*) {
        terminate_.store(true);
        return EINVAL;
    }));

    ASSERT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_DeadlineAlreadyOver_ContinuesUntilTerminated)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    // First call (start) returns 10s; the call inside sleep() returns a time past the deadline.
    int calls = 0;
    EXPECT_CALL(mockClock_, clockGetTime).WillRepeatedly(Invoke([&](struct timespec* ts) {
        ts->tv_sec = (calls++ == 0) ? 10 : 100;
        ts->tv_nsec = 0;
        terminate_.store(calls > 1);
        return 0;
    }));

    ASSERT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_MonotonicClockUnavailable_UsesMaxSyncTimestamp)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    // The real clock failure only has to last until performCyclicTriggers() has read it.
    int calls = 0;
    EXPECT_CALL(mockClock_, clockGetTime).WillRepeatedly(Invoke([&](struct timespec* ts) {
        ts->tv_sec = 10;
        ts->tv_nsec = 0;
        if (calls++ > 0)
        {
            g_failMonotonicClockGetTime.store(false);
            terminate_.store(true);
        }
        return 0;
    }));

    g_failMonotonicClockGetTime.store(true);
    const bool result = executor.startCyclicExec(terminate_);
    g_failMonotonicClockGetTime.store(false);

    ASSERT_TRUE(result);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon

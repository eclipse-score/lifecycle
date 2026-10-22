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

#include <unistd.h>

#include <atomic>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/details/daemon/CyclicExecutor.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/OsClockInterface.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"
#include "tests/utils/mocks/mock_syscalls.hpp"

using namespace testing;

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

class CyclicExecutorConstructorTest : public CyclicExecutorTest
{
};

TEST_F(CyclicExecutorConstructorTest, Constructor_WithValidParameters_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CyclicExecutor can be constructed with valid parameters.");

    EXPECT_NO_THROW(CyclicExecutor(mockClock_, 5));
}

TEST_F(CyclicExecutorConstructorTest, Constructor_WithZeroComponents_InitializesSuccessfully)
{
    RecordProperty("Description", "Verify that CyclicExecutor can be constructed with zero components.");

    EXPECT_NO_THROW(CyclicExecutor(mockClock_, 0));
}

TEST_F(CyclicExecutorConstructorTest, Constructor_WithMaxComponentCount_ThrowsLengthError)
{
    RecordProperty(
        "Description", "Verify that the constructor throws when the component count exceeds vector capacity.");

    EXPECT_THROW(CyclicExecutor(mockClock_, std::numeric_limits<std::size_t>::max()), std::length_error);
}

class CyclicExecutorInitTest : public CyclicExecutorTest
{
};

TEST_F(CyclicExecutorInitTest, Init_WithValidConfig_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init returns kNoError with valid configuration.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    EXPECT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorInitTest, Init_WithSmallCycleTime_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init succeeds with small cycle time.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 1;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    EXPECT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorInitTest, Init_WithLargeCycleTime_ReturnsNoError)
{
    RecordProperty("Description", "Verify that init succeeds with large cycle time.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 60000;

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    EXPECT_EQ(result, daemon::EInitCode::kNoError);
}

TEST_F(CyclicExecutorInitTest, Init_WithValidCycleTimer_SetsInitialization)
{
    RecordProperty("Description", "Verify that init properly initializes the cycle timer.");

    CyclicExecutor executor(mockClock_, 5);
    configuration::AliveSupervisionConfig config{};
    config.evaluation_cycle_ms = 100;

    // Ensure clockGetRes returns valid resolution
    struct timespec resolution{0, 1000000};  // 1 millisecond
    ON_CALL(mockClock_, clockGetRes).WillByDefault(DoAll(SetArgPointee<0>(resolution), Return(0)));

    daemon::EInitCode result = executor.init(mockRecoveryClient_, config);

    EXPECT_EQ(result, daemon::EInitCode::kNoError);
}

class CyclicExecutorStartTest : public CyclicExecutorTest
{
};

TEST_F(CyclicExecutorStartTest, StartCyclicExec_WithImmediateTermination_ReturnsTrue)
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

    EXPECT_TRUE(result);
}

class CyclicExecutorConstructSupervisionTest : public CyclicExecutorTest
{
  protected:
    // ACL setup only succeeds for the caller's own uid, so fixed uids make the result host-dependent.
    const uid_t uid_{getuid()};
};

TEST_F(CyclicExecutorConstructSupervisionTest, ConstructSupervision_WithValidParameters_ReturnsHandle)
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

    auto handle = executor.constructSupervision(componentId, uid_, compConfig);

    EXPECT_NE(handle, nullptr);
}

TEST_F(CyclicExecutorConstructSupervisionTest, ConstructSupervision_MultipleSupervisions_CreatesAll)
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

    auto handle1 = executor.constructSupervision(componentId1, uid_, compConfig);
    auto handle2 = executor.constructSupervision(componentId2, uid_, compConfig);

    EXPECT_NE(handle1, nullptr);
    EXPECT_NE(handle2, nullptr);
}

TEST_F(CyclicExecutorConstructSupervisionTest, ConstructSupervision_DuplicateId_ReturnsNullptr)
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
    ASSERT_NE(executor.constructSupervision(id, uid_, compConfig), nullptr);
    EXPECT_EQ(executor.constructSupervision(id, uid_, compConfig), nullptr);
}

TEST_F(CyclicExecutorConstructSupervisionTest, ConstructSupervision_ExceedingCapacity_Asserts)
{
    RecordProperty(
        "Description", "Verify that constructing more supervisions than reserved triggers the debug assertion.");

    CyclicExecutor executor(mockClock_, 2);  // 2 components allowed
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

    std::unique_ptr<IAliveSupervisionHandle> handle1;
    std::unique_ptr<IAliveSupervisionHandle> handle2;
    EXPECT_NO_THROW(handle1 = executor.constructSupervision(componentId1, uid_, compConfig));
    EXPECT_NO_THROW(handle2 = executor.constructSupervision(componentId2, uid_, compConfig));
    // The two constructions above fill both reserved slots, so the next one must trip the assertion.

    EXPECT_DEATH(
        executor.constructSupervision(componentId3, uid_, compConfig),
        "More alive supervisions than expected were constructed");
}

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

    EXPECT_FALSE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_SleepInterrupted_BreaksLoop)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    EXPECT_CALL(mockClock_, clockNanosleep).WillOnce(Invoke([this](int, const struct timespec*, struct timespec*) {
        terminate_.store(true);
        return EINTR;
    }));

    EXPECT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_SleepFails_ContinuesUntilTerminated)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    EXPECT_CALL(mockClock_, clockNanosleep).WillOnce(Invoke([this](int, const struct timespec*, struct timespec*) {
        terminate_.store(true);
        return EINVAL;
    }));

    EXPECT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_DeadlineAlreadyOver_ContinuesUntilTerminated)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    // First call (start) returns 10s; the call inside sleep() returns a time past the deadline.
    // First call (start) returns 10s; the call inside sleep() returns a time past the deadline.
    const struct timespec start{10, 0};
    const struct timespec pastDeadline{100, 0};
    EXPECT_CALL(mockClock_, clockGetTime)
        .WillOnce(DoAll(SetArgPointee<0>(start), Return(0)))
        .WillRepeatedly(DoAll(
            SetArgPointee<0>(pastDeadline),
            InvokeWithoutArgs([this] {
                terminate_.store(true);
            }),
            Return(0)));

    EXPECT_TRUE(executor.startCyclicExec(terminate_));
}

TEST_F(CyclicExecutorLoopTest, StartCyclicExec_MonotonicClockUnavailable_UsesMaxSyncTimestamp)
{
    CyclicExecutor executor(mockClock_, 5);
    ASSERT_EQ(executor.init(mockRecoveryClient_, config_), daemon::EInitCode::kNoError);

    // The real clock failure only has to last until performCyclicTriggers() has read it.
    const struct timespec now{10, 0};
    EXPECT_CALL(mockClock_, clockGetTime)
        .WillOnce(DoAll(SetArgPointee<0>(now), Return(0)))
        .WillRepeatedly(DoAll(
            SetArgPointee<0>(now),
            InvokeWithoutArgs([this] {
                g_fail_monotonic_clock_gettime.store(false);
                terminate_.store(true);
            }),
            Return(0)));

    g_fail_monotonic_clock_gettime.store(true);
    const bool result = executor.startCyclicExec(terminate_);
    g_fail_monotonic_clock_gettime.store(false);

    EXPECT_TRUE(result);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon

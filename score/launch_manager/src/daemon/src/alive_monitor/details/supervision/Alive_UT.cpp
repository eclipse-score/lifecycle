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

#include <memory>
#include <optional>

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;
using namespace std::chrono_literals;

namespace score::mw::lifecycle::internal::saf::supervision
{

using EStatus = Alive::EStatus;
using configuration::ComponentAliveSupervision;

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
        (const IdentifierHash& process_group_identifier, std::chrono::nanoseconds failure_timestamp),
        (noexcept, override));
};

class MockMonitorIfDaemon : public common::Observable<ifappl::Checkpoint>
{
  public:
    void PushCheckpointToObservers(ifappl::Checkpoint checkpoint)
    {
        pushResultToObservers(checkpoint);
    }
};

/// Helper: build a minimal Alive under test.
/// Owns all supporting objects so they outlive the Alive.
struct AliveFixture
{
    inline static const IdentifierHash kProcessId{"42U"};

    struct Builder
    {
        uint32_t failedCyclesTolerance = 0U;
        uint32_t minIndications = 1U;
        uint32_t maxIndications = 3U;
        uint32_t reportingCycleMs = 1U;

        Builder& withFailedCyclesTolerance(uint32_t val)
        {
            failedCyclesTolerance = val;
            return *this;
        }
        Builder& withMinIndications(uint32_t val)
        {
            minIndications = val;
            return *this;
        }
        Builder& withMaxIndications(uint32_t val)
        {
            maxIndications = val;
            return *this;
        }
        Builder& withReportingCycleMs(uint32_t val)
        {
            reportingCycleMs = val;
            return *this;
        }

        [[nodiscard]] AliveFixture build() const
        {
            return AliveFixture{*this};
        }
    };

    const IdentifierHash kProcessIdentifier{"test_proc"};

    std::shared_ptr<MockRecoveryClient> mockClient = std::make_shared<MockRecoveryClient>();

    ifexm::ObservableEvent processState;
    MockMonitorIfDaemon mock_monitor;

    std::unique_ptr<Alive> alive;

    explicit AliveFixture(const Builder& bld) : processState(kProcessId), mock_monitor()
    {
        ComponentAliveSupervision cfg{};
        cfg.min_indications = bld.minIndications;
        cfg.max_indications = bld.maxIndications;
        cfg.failed_cycles_tolerance = bld.failedCyclesTolerance;
        cfg.reporting_cycle_ms = bld.reportingCycleMs;

        alive = std::make_unique<Alive>(
            kProcessIdentifier, cfg, mockClient, mock_monitor, internal::kDefaultAliveSupCheckpointBufferElements);
        processState.attachObserver(*alive);
    }

    /// Send an activation event and notify observers.
    void activateProcess(std::chrono::nanoseconds ts)
    {
        processState.event.systemClockTimestamp.tv_nsec = ts.count();
        processState.event.eventType = SupervisionEventType::kActivation;
        processState.pushData();
    }

    /// Send a deactivation event and notify observers.
    void deactivateProcess(std::chrono::nanoseconds ts)
    {
        processState.event.systemClockTimestamp.tv_nsec = ts.count();
        processState.event.eventType = SupervisionEventType::kDeactivation;
        processState.pushData();
    }

    /// Report one alive heartbeat checkpoint at the given timestamp.
    void reportHeartbeat(std::chrono::nanoseconds timestamp)
    {
        mock_monitor.PushCheckpointToObservers(ifappl::Checkpoint{timestamp});
    }
};

class AliveSupervisionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

TEST_F(AliveSupervisionTest, AliveTransitionsOkToExpiredOnMissingHeartbeat)
{
    RecordProperty(
        "Description",
        "Verify that Alive transitions from deactivated -> ok -> expired when no heartbeats are reported "
        "and failedCyclesTolerance == 0. sendRecoveryRequest must be called exactly once with the "
        "configured recovery target hash and the end of the missed reference cycle as failure timestamp.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    const std::chrono::nanoseconds referenceCycleEnd = 1ms + 10ns;

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier, referenceCycleEnd))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    fix.alive->evaluate(referenceCycleEnd);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, AliveStaysOkWithCorrectHeartbeats)
{
    RecordProperty(
        "Description", "Verify that sending at least minIndications heartbeats per cycle keeps Alive in ok.");
    AliveFixture fix = AliveFixture::Builder{}.withReportingCycleMs(1).build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_, _)).Times(0);

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getTimestamp(), 10ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Cycle 1: one heartbeat at t=500 (within [10, 1010]), evaluate at t=1011
    fix.reportHeartbeat(500ns);
    fix.alive->evaluate(1ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Cycle 2: one heartbeat at t=1500 (within [1010, 2010]), evaluate at t=2011
    fix.reportHeartbeat(1ms + 500ns);
    fix.alive->evaluate(2ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);
}

TEST_F(AliveSupervisionTest, AliveReportsEnqueueFailureWhenRingBufferFull)
{
    RecordProperty(
        "Description",
        "Verify that when sendRecoveryRequest fails (ring buffer full), hasRecoveryEnqueueFailed reports true. "
        "The recovery request carries the end of the missed reference cycle as failure timestamp.");

    AliveFixture fix = AliveFixture::Builder{}.build();

    const std::chrono::nanoseconds referenceCycleEnd = 1ms + 10ns;

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier, referenceCycleEnd))
        .Times(1)
        .WillOnce(Return(false));

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);

    EXPECT_FALSE(fix.alive->hasRecoveryEnqueueFailed());

    fix.alive->evaluate(referenceCycleEnd + 1ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
    EXPECT_TRUE(fix.alive->hasRecoveryEnqueueFailed());
}

TEST_F(AliveSupervisionTest, AliveDebouncesThroughFailedBeforeExpired)
{
    RecordProperty(
        "Description",
        "Verify that failedCyclesTolerance debouncing works: with tolerance=1 the supervision passes "
        "through failed before reaching expired. The failure timestamp is the end of the second missed reference "
        "cycle, i.e. the cycle in which the tolerance is exceeded.");
    AliveFixture fix = AliveFixture::Builder{}.withFailedCyclesTolerance(1U).build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier, 2ms + 10ns))
        .Times(1)
        .WillOnce(::testing::Return(true));

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // First missed cycle: ok -> failed (tolerance not yet exceeded)
    fix.alive->evaluate(1ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    // Second missed cycle: tolerance exceeded -> expired
    fix.alive->evaluate(2ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, CanRecoverFromFailed)
{
    RecordProperty("Description", "Verify that a heartbeat within the tolerance after failed recovers the supervision");
    AliveFixture fix =
        AliveFixture::Builder{}.withMinIndications(2).withMaxIndications(2).withFailedCyclesTolerance(2U).build();

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // First missed cycle: ok -> failed (tolerance not yet exceeded)
    fix.alive->evaluate(1ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    fix.reportHeartbeat(1ms + 500ns);

    // Next cycle has one heartbeat: still in failed
    fix.alive->evaluate(2ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    fix.reportHeartbeat(2ms + 500ns);
    fix.reportHeartbeat(2ms + 1000ns);

    // Next cycle has two heartbeats: failed cycles decreases to 1
    fix.alive->evaluate(3ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    fix.reportHeartbeat(3ms + 500ns);
    fix.reportHeartbeat(3ms + 1000ns);

    // Next cycle has two heartbeats: failed cycles decreases to 0, failed -> ok
    fix.alive->evaluate(4ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);
}

TEST_F(AliveSupervisionTest, CanExpireFromTooManyHeartbeatsInFailed)
{
    RecordProperty(
        "Description", "Verify that an excess of heartbeats within the tolerance after failed results in expiration");
    AliveFixture fix = AliveFixture::Builder{}.withMaxIndications(1).withFailedCyclesTolerance(1U).build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest).Times(1).WillOnce(Return(true));

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // First missed cycle: ok -> failed (tolerance not yet exceeded)
    fix.alive->evaluate(1ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    fix.reportHeartbeat(1ms + 500ns);
    fix.reportHeartbeat(1ms + 1000ns);

    // Next cycle has two heartbeats: failed -> expired
    fix.alive->evaluate(2ms + 11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, DeactivatesOnSupervisionDeactivation)
{
    RecordProperty("Description", "Verify that a deactivation event deactivates the supervision from ok.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_, _)).Times(0);

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    fix.deactivateProcess(20ns);
    fix.alive->evaluate(21ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);
}

TEST_F(AliveSupervisionTest, ReactivatesAfterDeactivation)
{
    RecordProperty(
        "Description",
        "Verify that after a deactivation the supervision can be reactivated when an activation"
        " event is received again.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_, _)).Times(0);

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    fix.deactivateProcess(20ns);
    fix.alive->evaluate(21ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);

    fix.activateProcess(30ns);
    fix.alive->evaluate(31ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);
}

TEST_F(AliveSupervisionTest, MaxIndicationViolationExpires)
{
    RecordProperty(
        "Description",
        "Verify that exceeding the maximum allowed heartbeats per cycle leads to failure. The failure timestamp "
        "is the end of the reference cycle in which the maximum was exceeded.");
    // max=1, tolerance=0: more than 1 heartbeat per cycle expires immediately
    AliveFixture fix = AliveFixture::Builder{}.withMaxIndications(1U).build();

    const std::chrono::nanoseconds referenceCycleEnd = 1ms + 10ns;

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier, referenceCycleEnd))
        .Times(1)
        .WillOnce(Return(true));

    fix.activateProcess(10ns);
    fix.alive->evaluate(11ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Two heartbeats in one cycle violates max=1
    fix.reportHeartbeat(100ns);
    fix.reportHeartbeat(200ns);
    fix.alive->evaluate(referenceCycleEnd);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, CheckpointDataLossEvent)
{
    RecordProperty("Description", "Test that a checkpoint with missing data causes the supervision to expire");

    AliveFixture fix = AliveFixture::Builder{}.build();
    const ifappl::Checkpoint dataLossCheckpoint = ifappl::Checkpoint{0ns};
    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest).Times(1).WillOnce(Return(true));

    fix.alive->updateData(dataLossCheckpoint);
    fix.alive->evaluate(1ms);

    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, TimeOverflowCausesExpiration)
{
    RecordProperty("Description", "Verify that when a timestamp would produce overflow, the supervision expires");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest).Times(1).WillOnce(Return(true));

    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);

    fix.activateProcess(std::chrono::nanoseconds::max() - 1ns);

    fix.alive->evaluate(10ns);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

}  // namespace score::mw::lifecycle::internal::saf::supervision

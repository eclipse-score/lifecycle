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
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_event_queue.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

using namespace score::mw::lifecycle;
using namespace score::mw::lifecycle::internal::saf;

namespace
{

class RecordingObserver : public common::Observer<ifexm::ObservableEvent>
{
  public:
    explicit RecordingObserver(IdentifierHash id) : id_(id)
    {
    }

    void updateData(const ifexm::ObservableEvent& observable) noexcept override
    {
        received.push_back(observable.event);
    }

    IdentifierHash getIdentifier() const noexcept override
    {
        return id_;
    }

    std::vector<SupervisionEvent> received;

  private:
    IdentifierHash id_;
};

constexpr std::chrono::nanoseconds kFarFuture{std::chrono::hours{24 * 365 * 100}};

}  // namespace

class ObservableEventReader_UT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        buffer_ = std::make_shared<SupervisionBufferType>(2);
        reader_ = std::make_unique<ifexm::ObservableEventReader>(buffer_);
        ASSERT_TRUE(reader_->registerObservableEvent(observable_, process_));
        observable_.attachObserver(observer_);
    }

    SupervisionEvent makeEvent(SupervisionEventType type)
    {
        SupervisionEvent e{};
        e.id = process_;
        e.eventType = type;
        clock_gettime(CLOCK_REALTIME, &e.systemClockTimestamp);
        return e;
    }

    SupervisionEvent makeEventAt(SupervisionEventType type, std::time_t seconds)
    {
        SupervisionEvent e{};
        e.id = process_;
        e.eventType = type;
        e.systemClockTimestamp = timespec{seconds, 0};
        return e;
    }

    const IdentifierHash process_{"Process"};
    std::shared_ptr<SupervisionBufferType> buffer_;
    ifexm::ObservableEvent observable_{process_};
    RecordingObserver observer_{process_};
    std::unique_ptr<ifexm::ObservableEventReader> reader_;
};

TEST_F(ObservableEventReader_UT, DistributeChanges_EmptyQueue_SucceedsWithoutUpdates)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    EXPECT_TRUE(observer_.received.empty());
}

TEST_F(ObservableEventReader_UT, DistributeChanges_QueuedEvents_DeliveredInOrder)
{
    RecordProperty("DerivationTechnique", "requirements-based");
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kActivation)));
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kDeactivation)));

    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    ASSERT_EQ(observer_.received.size(), 2U);
    EXPECT_EQ(observer_.received[0].eventType, SupervisionEventType::kActivation);
    EXPECT_EQ(observer_.received[1].eventType, SupervisionEventType::kDeactivation);

    observer_.received.clear();
    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    EXPECT_TRUE(observer_.received.empty());
}

TEST_F(ObservableEventReader_UT, RegisterObservableEvent_DuplicateId_ReturnsFalse)
{
    RecordProperty("DerivationTechnique", "error-condition-injection");
    ifexm::ObservableEvent duplicate{process_};
    EXPECT_FALSE(reader_->registerObservableEvent(duplicate, process_));
}

TEST_F(ObservableEventReader_UT, DeregisterObservableEvent_RegisteredId_StopsDelivery)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    reader_->deregisterObservableEvent(process_);
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kActivation)));

    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    EXPECT_TRUE(observer_.received.empty());
}

TEST_F(ObservableEventReader_UT, DeregisterObservableEvent_UnknownId_IsNoOp)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    reader_->deregisterObservableEvent(IdentifierHash{"Unknown"});
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kActivation)));

    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    EXPECT_EQ(observer_.received.size(), 1U);
}

TEST_F(ObservableEventReader_UT, DistributeChanges_EventAfterSyncTimestamp_DeliveredInNextCycle)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    ASSERT_TRUE(buffer_->tryEnqueue(makeEventAt(SupervisionEventType::kActivation, 10)));

    EXPECT_TRUE(reader_->distributeChanges(std::chrono::seconds{5}));
    EXPECT_TRUE(observer_.received.empty());

    EXPECT_TRUE(reader_->distributeChanges(std::chrono::seconds{5}));
    ASSERT_EQ(observer_.received.size(), 1U);
    EXPECT_EQ(observer_.received[0].eventType, SupervisionEventType::kActivation);
}

TEST_F(ObservableEventReader_UT, DistributeChanges_EventAfterSyncTimestamp_LaterEventsStayQueued)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    ASSERT_TRUE(buffer_->tryEnqueue(makeEventAt(SupervisionEventType::kActivation, 10)));
    ASSERT_TRUE(buffer_->tryEnqueue(makeEventAt(SupervisionEventType::kDeactivation, 11)));

    EXPECT_TRUE(reader_->distributeChanges(std::chrono::seconds{5}));
    EXPECT_TRUE(observer_.received.empty());

    // Next cycle: pending event is pushed first, then the queued event is read and deferred again.
    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    ASSERT_EQ(observer_.received.size(), 2U);
    EXPECT_EQ(observer_.received[0].eventType, SupervisionEventType::kActivation);
    EXPECT_EQ(observer_.received[1].eventType, SupervisionEventType::kDeactivation);
}

TEST_F(ObservableEventReader_UT, DistributeChanges_Overflow_ReturnsFalse_AndStaysFailedUntilReset)
{
    RecordProperty("DerivationTechnique", "error-condition-injection");
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kActivation)));
    ASSERT_TRUE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kDeactivation)));
    EXPECT_FALSE(buffer_->tryEnqueue(makeEvent(SupervisionEventType::kActivation)));

    EXPECT_FALSE(reader_->distributeChanges(kFarFuture));
    EXPECT_TRUE(observer_.received.empty());

    // Sticky: still failing on subsequent cycles, queued events remain undelivered.
    EXPECT_FALSE(reader_->distributeChanges(kFarFuture));
    EXPECT_TRUE(observer_.received.empty());

    // After the flag is cleared, queued events are delivered again.
    EXPECT_TRUE(buffer_->getOverflowFlag(true));
    EXPECT_TRUE(reader_->distributeChanges(kFarFuture));
    EXPECT_EQ(observer_.received.size(), 2U);
}

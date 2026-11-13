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
#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_handle.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace score::mw::lifecycle;

class SupervisionControlClient_UT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        buffer_ = std::make_shared<SupervisionBufferType>();
        handle_ = std::make_unique<SupervisionHandle>(process_, buffer_);
    }

    void TearDown() override
    {
        handle_.reset();
        buffer_.reset();
    }

    const IdentifierHash process_{"Process"};
    std::shared_ptr<SupervisionBufferType> buffer_;
    std::unique_ptr<IAliveSupervisionHandle> handle_;
};

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueOneEvent_Succeeds)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "This test verifies that a single SupervisionEvent can be successfully queued using the "
        "SupervisionControlNotifier and retrieved using the SupervisionControlReceiver.");
    SupervisionEvent event1{process_, SupervisionEventType::kActivation, {}};

    clock_gettime(CLOCK_MONOTONIC, &event1.systemClockTimestamp);

    bool queued = handle_->activateSupervision(event1.systemClockTimestamp);
    ASSERT_TRUE(queued);

    SupervisionEvent result;
    ASSERT_TRUE(buffer_->tryDequeue(result));

    EXPECT_EQ(result.id, event1.id);
    EXPECT_EQ(result.eventType, event1.eventType);
    EXPECT_EQ(result.systemClockTimestamp.tv_nsec, event1.systemClockTimestamp.tv_nsec);

    bool items_remaining = buffer_->tryDequeue(result);
    EXPECT_FALSE(items_remaining);
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueDeactivation_Succeeds)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "This test verifies that a deactivation request is queued as a kDeactivation event.");
    timespec time{};
    clock_gettime(CLOCK_MONOTONIC, &time);

    ASSERT_TRUE(handle_->deactivateSupervision(time));

    SupervisionEvent result;
    ASSERT_TRUE(buffer_->tryDequeue(result));
    EXPECT_EQ(result.id, process_);
    EXPECT_EQ(result.eventType, SupervisionEventType::kDeactivation);
    EXPECT_EQ(result.systemClockTimestamp.tv_nsec, time.tv_nsec);
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_GetConnectionId_IsNotEmpty)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "This test verifies that the handle exposes a non-empty IPC connection id.");
    EXPECT_FALSE(handle_->getConnectionId().empty());
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueMaxNumberOfEvents_Succeeds)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "This test verifies that the SupervisionControlNotifier can successfully queue the maximum number of "
        "SupervisionEvent "
        "instances defined by the buffer size, and that they can be retrieved using the SupervisionControlReceiver.");

    SupervisionEvent event{process_, SupervisionEventType::kActivation, {}};

    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        bool queued = handle_->activateSupervision(event.systemClockTimestamp);
        ASSERT_TRUE(queued) << "Failed to queue event at index " << i;
    }

    SupervisionEvent result;

    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        ASSERT_TRUE(buffer_->tryDequeue(result));
        EXPECT_EQ(result.id, event.id);
    }

    bool items_remaining = buffer_->tryDequeue(result);
    EXPECT_FALSE(items_remaining);
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueOneEventTooMany_Fails)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "This test verifies that attempting to queue a SupervisionEvent when the buffer is already at maximum capacity "
        "results in a failure, and that no additional events can be retrieved from the receiver.");
    SupervisionEvent event{process_, SupervisionEventType::kActivation, {}};

    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        bool queued = handle_->activateSupervision(event.systemClockTimestamp);
        ASSERT_TRUE(queued) << "Failed to queue event at index " << i;
    }

    bool queued = handle_->activateSupervision(event.systemClockTimestamp);
    EXPECT_FALSE(queued) << "Expected queuing to fail due to full buffer";
}

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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/mock_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/sequence_ready_condition.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;
using namespace std::chrono_literals;

const ProcessHandle kHandle{42, nullptr};

MATCHER(IsTestHandle, "")
{
    const ProcessHandle* const process = std::get_if<ProcessHandle>(&arg);
    return (process != nullptr) && (process->pid == kHandle.pid);
}

class SequenceReadyConditionTest : public Test
{
  protected:
    SequenceReadyConditionTest()
    {
        RecordProperty("TestType", "interface-test");

        auto first = std::make_unique<StrictMock<MockReadyCondition>>();
        auto second = std::make_unique<StrictMock<MockReadyCondition>>();
        first_ = first.get();
        second_ = second.get();

        std::vector<std::unique_ptr<const IReadyCondition>> stages;
        stages.push_back(std::move(first));
        stages.push_back(std::move(second));
        sequence_ = std::make_unique<SequenceReadyCondition>(std::move(stages));
    }

    StrictMock<MockReadyCondition>* first_{nullptr};
    StrictMock<MockReadyCondition>* second_{nullptr};
    std::unique_ptr<SequenceReadyCondition> sequence_;
};

TEST_F(SequenceReadyConditionTest, WaitsForAllStagesInOrder)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions. "
        "When the sequence is waited for and both stages are fulfilled. "
        "Then the stages are waited for in order on the same resource, the first one with the complete timeout, "
        "and the wait succeeds.");

    {
        const InSequence in_sequence;
        EXPECT_CALL(*first_, wait(_, IsTestHandle(), 200ms)).WillOnce(Return(Result<void>{}));
        EXPECT_CALL(*second_, wait(_, IsTestHandle(), AllOf(Le(200ms), Ge(0ms)))).WillOnce(Return(Result<void>{}));
    }

    EXPECT_TRUE(sequence_->wait(cpp::stop_token{}, kHandle, 200ms).has_value());
}

TEST_F(SequenceReadyConditionTest, LaterStageGetsRemainingTimeout)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions and a timeout of 200ms. "
        "When the first stage takes 50ms. "
        "Then the second stage is waited for with at most the remaining 150ms.");

    const InSequence in_sequence;
    EXPECT_CALL(*first_, wait(_, _, 200ms)).WillOnce(Invoke([](auto&&...) {
        std::this_thread::sleep_for(50ms);
        return Result<void>{};
    }));
    EXPECT_CALL(*second_, wait(_, _, AllOf(Le(150ms), Gt(0ms)))).WillOnce(Return(Result<void>{}));

    EXPECT_TRUE(sequence_->wait(cpp::stop_token{}, kHandle, 200ms).has_value());
}

TEST_F(SequenceReadyConditionTest, LaterStageIsCheckedOnceWhenTimeoutIsUsedUp)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions and a timeout of 20ms. "
        "When the first stage is fulfilled only after the timeout. "
        "Then the second stage is still waited for, with a timeout of zero, so that it is checked once.");

    const InSequence in_sequence;
    EXPECT_CALL(*first_, wait(_, _, 20ms)).WillOnce(Invoke([](auto&&...) {
        std::this_thread::sleep_for(30ms);
        return Result<void>{};
    }));
    EXPECT_CALL(*second_, wait(_, _, 0ms)).WillOnce(Return(Result<void>{}));

    EXPECT_TRUE(sequence_->wait(cpp::stop_token{}, kHandle, 20ms).has_value());
}

TEST_F(SequenceReadyConditionTest, FailingStageStopsTheSequenceWithItsError)
{
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions. "
        "When the first stage fails. "
        "Then the second stage is not waited for, and the error of the first stage is returned unchanged.");

    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Return(MakeUnexpected(ExecErrc::kNotImplemented)));

    const Result<void> result = sequence_->wait(cpp::stop_token{}, kHandle, 200ms);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kNotImplemented);
}

TEST_F(SequenceReadyConditionTest, StopRequestedDuringStageStillChecksLaterStages)
{
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions. "
        "When a stop is requested while the first stage is waited for, the first stage succeeds anyway, and the "
        "second stage is already fulfilled. "
        "Then the second stage is still checked with the stopped token and the wait succeeds.");

    cpp::stop_source stop_source;
    const InSequence in_sequence;
    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Invoke([&stop_source](auto&&...) {
        static_cast<void>(stop_source.request_stop());
        return Result<void>{};
    }));
    EXPECT_CALL(*second_, wait(Property(&cpp::stop_token::stop_requested, true), IsTestHandle(), _))
        .WillOnce(Return(Result<void>{}));

    EXPECT_TRUE(sequence_->wait(stop_source.get_token(), kHandle, 200ms).has_value());
}

TEST_F(SequenceReadyConditionTest, StopRequestedDuringStageFailsWithErrorOfUnfulfilledStage)
{
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions. "
        "When a stop is requested while the first stage is waited for, and the second stage is not fulfilled. "
        "Then the wait fails with the error of the second stage.");

    cpp::stop_source stop_source;
    const InSequence in_sequence;
    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Invoke([&stop_source](auto&&...) {
        static_cast<void>(stop_source.request_stop());
        return Result<void>{};
    }));
    EXPECT_CALL(*second_, wait(_, _, _)).WillOnce(Return(Result<void>{MakeUnexpected(ExecErrc::kGeneralError)}));

    const Result<void> result = sequence_->wait(stop_source.get_token(), kHandle, 200ms);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(SequenceReadyConditionTest, StopRequestedBeforeWaitSucceedsIfAllStagesFulfilled)
{
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions and an already stopped stop token. "
        "When both stages are already fulfilled. "
        "Then every stage is checked and the wait succeeds, since a stop only interrupts waiting.");

    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    const InSequence in_sequence;
    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Return(Result<void>{}));
    EXPECT_CALL(*second_, wait(_, _, _)).WillOnce(Return(Result<void>{}));

    EXPECT_TRUE(sequence_->wait(stop_source.get_token(), kHandle, 200ms).has_value());
}

TEST_F(SequenceReadyConditionTest, PrepareReachesAllStages)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions. "
        "When the sequence is prepared for a new activation. "
        "Then every stage is prepared.");

    EXPECT_CALL(*first_, prepare());
    EXPECT_CALL(*second_, prepare());

    sequence_->prepare();
}

TEST_F(SequenceReadyConditionTest, TerminationBeforeWaitReachesAllStages)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions which is not waited for yet. "
        "When the process terminates. "
        "Then the termination is forwarded to all stages, since none of them is fulfilled yet.");

    EXPECT_CALL(*first_, handleTermination(IsTestHandle(), 3));
    EXPECT_CALL(*second_, handleTermination(IsTestHandle(), 3));

    sequence_->handleTermination(kHandle, 3);
}

TEST_F(SequenceReadyConditionTest, TerminationDuringLaterStageSkipsFulfilledStages)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions whose first stage is fulfilled. "
        "When the process terminates while the second stage is waited for. "
        "Then the termination is only forwarded to the second stage.");

    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Return(Result<void>{}));
    EXPECT_CALL(*second_, wait(_, _, _)).WillOnce(Invoke([this](auto&&...) {
        // Termination notifications come from another thread in reality.
        std::thread termination_thread([this]() {
            sequence_->handleTermination(kHandle, 0);
        });
        termination_thread.join();
        return Result<void>{};
    }));
    // The strict mock fails the test if the first stage is notified.
    EXPECT_CALL(*second_, handleTermination(_, 0));

    EXPECT_TRUE(sequence_->wait(cpp::stop_token{}, kHandle, 200ms).has_value());
}

TEST_F(SequenceReadyConditionTest, TerminationFailingTheCurrentStageEndsTheSequence)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions whose first stage is waited for. "
        "When the process terminates and the first stage fails because of it. "
        "Then both stages are notified, the second stage is not waited for and the error is returned.");

    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Invoke([this](auto&&...) {
        std::thread termination_thread([this]() {
            sequence_->handleTermination(kHandle, 1);
        });
        termination_thread.join();
        return Result<void>{MakeUnexpected(ExecErrc::kGeneralError)};
    }));
    EXPECT_CALL(*first_, handleTermination(_, 1));
    EXPECT_CALL(*second_, handleTermination(_, 1));

    const Result<void> result = sequence_->wait(cpp::stop_token{}, kHandle, 200ms);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(SequenceReadyConditionTest, TerminationAfterWaitReachesAllStagesAgain)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a sequence of two ready conditions whose wait ended in the second stage. "
        "When the process of the next activation terminates before the sequence is waited for again. "
        "Then the termination is forwarded to all stages again.");

    EXPECT_CALL(*first_, wait(_, _, _)).WillOnce(Return(Result<void>{}));
    EXPECT_CALL(*second_, wait(_, _, _)).WillOnce(Return(MakeUnexpected(ExecErrc::kGeneralError)));
    ASSERT_FALSE(sequence_->wait(cpp::stop_token{}, kHandle, 200ms).has_value());

    EXPECT_CALL(*first_, handleTermination(_, _));
    EXPECT_CALL(*second_, handleTermination(_, _));

    sequence_->handleTermination(kHandle, 1);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

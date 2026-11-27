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

#include "score/health_monitor/deadline.h"

#include "score/health_monitor/error.h"

#include <gtest/gtest.h>

#include <utility>

using namespace score::health_monitor;

namespace
{

class CountingDeadline final : public Deadline
{
  public:
    int start_calls{0};
    int stop_calls{0};
    score::Result<void> start_result{};

  private:
    score::Result<void> DoStart() noexcept override
    {
        ++start_calls;
        return start_result;
    }

    void DoStop() noexcept override
    {
        ++stop_calls;
    }
};

class DeadlineHandleTest : public ::testing::Test
{
  protected:
    CountingDeadline deadline_;
};

}  // namespace

TEST_F(DeadlineHandleTest, StartReturnsHandleAndCallsDoStartOnce)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that starting a deadline returns a handle and starts the deadline once.");

    auto handle{deadline_.Start()};

    ASSERT_TRUE(handle.has_value());
    EXPECT_EQ(deadline_.start_calls, 1);
    EXPECT_EQ(deadline_.stop_calls, 0);
}

TEST_F(DeadlineHandleTest, SecondStartWhileHandleExistsReturnsDeadlineRunning)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that starting a running deadline again fails with kDeadlineRunning.");

    auto first{deadline_.Start()};
    ASSERT_TRUE(first.has_value());

    const auto second{deadline_.Start()};

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error(), MakeError(ErrorCode::kDeadlineRunning));
    EXPECT_EQ(deadline_.start_calls, 1);
    EXPECT_EQ(deadline_.stop_calls, 0);
}

TEST_F(DeadlineHandleTest, StopCallsDoStopOnce)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that stopping a handle stops the deadline once.");

    auto handle{deadline_.Start()};
    ASSERT_TRUE(handle.has_value());

    handle.value().Stop();

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, SecondStopDoesNothing)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a second stop of the same handle does not stop the deadline again.");

    auto handle{deadline_.Start()};
    ASSERT_TRUE(handle.has_value());

    handle.value().Stop();
    handle.value().Stop();

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, DestructorStopsDeadline)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that destroying a handle stops the deadline.");

    {
        auto handle{deadline_.Start()};
        ASSERT_TRUE(handle.has_value());
    }

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, DestructorAfterStopDoesNotStopAgain)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that destroying a stopped handle does not stop the deadline again.");

    {
        auto handle{deadline_.Start()};
        ASSERT_TRUE(handle.has_value());
        handle.value().Stop();
    }

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, MovedFromHandleDoesNothingAndMovedToHandleStopsOnce)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that only the moved-to handle stops the deadline, once.");

    auto started{deadline_.Start()};
    ASSERT_TRUE(started.has_value());
    {
        DeadlineHandle moved_from{std::move(started).value()};
        {
            DeadlineHandle moved_to{std::move(moved_from)};
            moved_from.Stop();
            EXPECT_EQ(deadline_.stop_calls, 0);
        }

        EXPECT_EQ(deadline_.stop_calls, 1);
    }

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, StartAfterStopWorksAgain)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a stopped deadline can be started again.");

    {
        auto first{deadline_.Start()};
        ASSERT_TRUE(first.has_value());
        first.value().Stop();
    }

    auto second{deadline_.Start()};

    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(deadline_.start_calls, 2);
}

TEST_F(DeadlineHandleTest, StartAfterTheHandleDestructorWorksAgain)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_no_stop, comp_req__health_monitor__dl_late_stop");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a deadline whose handle was destroyed can be started again.");

    {
        auto first{deadline_.Start()};
        ASSERT_TRUE(first.has_value());
    }
    ASSERT_EQ(deadline_.stop_calls, 1);

    auto second{deadline_.Start()};

    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(deadline_.start_calls, 2);
    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, DoStartErrorIsPassedThrough)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_failed_reuse");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a start error of the deadline reaches the caller without a stop.");

    deadline_.start_result = score::MakeUnexpected(ErrorCode::kDeadlineFailed);

    const auto handle{deadline_.Start()};

    ASSERT_FALSE(handle.has_value());
    EXPECT_EQ(handle.error(), MakeError(ErrorCode::kDeadlineFailed));
    EXPECT_EQ(deadline_.stop_calls, 0);
}

TEST_F(DeadlineHandleTest, DoStartErrorLeavesDeadlineStartable)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_failed_reuse");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a failed start does not mark the deadline as running.");

    deadline_.start_result = score::MakeUnexpected(ErrorCode::kDeadlineFailed);
    const auto failed{deadline_.Start()};
    ASSERT_FALSE(failed.has_value());

    deadline_.start_result = {};
    const auto retried{deadline_.Start()};

    ASSERT_TRUE(retried.has_value());
    EXPECT_EQ(deadline_.start_calls, 2);
}

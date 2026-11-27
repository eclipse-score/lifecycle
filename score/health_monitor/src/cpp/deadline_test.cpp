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
    auto handle{deadline_.Start()};

    ASSERT_TRUE(handle.has_value());
    EXPECT_EQ(deadline_.start_calls, 1);
    EXPECT_EQ(deadline_.stop_calls, 0);
}

TEST_F(DeadlineHandleTest, SecondStartWhileHandleExistsReturnsDeadlineRunning)
{
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
    auto handle{deadline_.Start()};
    ASSERT_TRUE(handle.has_value());

    handle.value().Stop();

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, SecondStopDoesNothing)
{
    auto handle{deadline_.Start()};
    ASSERT_TRUE(handle.has_value());

    handle.value().Stop();
    handle.value().Stop();

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, DestructorStopsDeadline)
{
    {
        auto handle{deadline_.Start()};
        ASSERT_TRUE(handle.has_value());
    }

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, DestructorAfterStopDoesNotStopAgain)
{
    {
        auto handle{deadline_.Start()};
        ASSERT_TRUE(handle.has_value());
        handle.value().Stop();
    }

    EXPECT_EQ(deadline_.stop_calls, 1);
}

TEST_F(DeadlineHandleTest, MovedFromHandleDoesNothingAndMovedToHandleStopsOnce)
{
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
    {
        auto first{deadline_.Start()};
        ASSERT_TRUE(first.has_value());
        first.value().Stop();
    }

    auto second{deadline_.Start()};

    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(deadline_.start_calls, 2);
}

TEST_F(DeadlineHandleTest, DoStartErrorIsPassedThrough)
{
    deadline_.start_result = score::MakeUnexpected(ErrorCode::kDeadlineFailed);

    const auto handle{deadline_.Start()};

    ASSERT_FALSE(handle.has_value());
    EXPECT_EQ(handle.error(), MakeError(ErrorCode::kDeadlineFailed));
    EXPECT_EQ(deadline_.stop_calls, 0);
}

TEST_F(DeadlineHandleTest, DoStartErrorLeavesDeadlineStartable)
{
    deadline_.start_result = score::MakeUnexpected(ErrorCode::kDeadlineFailed);
    const auto failed{deadline_.Start()};
    ASSERT_FALSE(failed.has_value());

    deadline_.start_result = {};
    const auto retried{deadline_.Start()};

    ASSERT_TRUE(retried.has_value());
    EXPECT_EQ(deadline_.start_calls, 2);
}

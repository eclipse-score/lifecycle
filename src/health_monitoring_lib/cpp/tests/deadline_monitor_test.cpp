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

#include "score/hm/deadline/deadline_monitor.h"
#include "score/hm/health_monitor.h"
#include <gtest/gtest.h>

using namespace score::hm;
using namespace score::hm::deadline;

TEST(DeadlineMonitorBuilder, New_Succeeds)
{
    DeadlineMonitorBuilder deadline_monitor_builder;
}

TEST(DeadlineMonitorBuilder, AddDeadline_Succeeds)
{
    using namespace std::chrono_literals;
    DeadlineTag deadline_tag{"deadline"};
    TimeRange range{50ms, 150ms};
    auto deadline_monitor_builder{DeadlineMonitorBuilder{}.add_deadline(deadline_tag, range)};
}

class DeadlineMonitorFixture : public ::testing::Test
{
  protected:
    std::optional<DeadlineMonitor> deadline_monitor_;

    void SetUp() override
    {
        // Monitor must be obtained from HMON.
        // Initialize deadline monitor builder.
        using namespace std::chrono_literals;
        MonitorTag deadline_monitor_tag{"deadline_monitor"};
        DeadlineTag deadline_tag{"deadline"};
        TimeRange range{50ms, 150ms};
        auto deadline_monitor_builder{DeadlineMonitorBuilder{}.add_deadline(deadline_tag, range)};

        // Build HMON, including deadline monitor.
        auto hmon_build_result{HealthMonitorBuilder{}
                                   .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                                   .build()};
        ASSERT_TRUE(hmon_build_result.has_value());
        auto hmon{std::move(hmon_build_result.value())};

        // Get deadline monitor.
        auto get_deadline_monitor_result{hmon.get_deadline_monitor(deadline_monitor_tag)};
        ASSERT_TRUE(get_deadline_monitor_result.has_value());
        deadline_monitor_ = std::move(get_deadline_monitor_result.value());
    }
};

TEST_F(DeadlineMonitorFixture, GetDeadline_Succeeds)
{
    // Get deadline.
    auto get_deadline_result{deadline_monitor_->get_deadline(DeadlineTag{"deadline"})};
    ASSERT_TRUE(get_deadline_result.has_value());
}

TEST_F(DeadlineMonitorFixture, GetDeadline_Unknown)
{
    // Get deadline.
    auto get_deadline_result{deadline_monitor_->get_deadline(DeadlineTag{"unknown"})};
    ASSERT_FALSE(get_deadline_result.has_value());
    ASSERT_EQ(get_deadline_result.error(), Error::NotFound);
}

class DeadlineFixture : public DeadlineMonitorFixture
{
};

TEST_F(DeadlineFixture, Start_Succeeds)
{
    // Get deadline.
    DeadlineTag deadline_tag{"deadline"};
    auto get_deadline_result{deadline_monitor_->get_deadline(deadline_tag)};
    ASSERT_TRUE(get_deadline_result.has_value());
    auto deadline{std::move(get_deadline_result.value())};

    // Try to start and stop deadline.
    auto deadline_start_result{deadline.start()};
    ASSERT_TRUE(deadline_start_result.has_value());
    auto deadline_handle{std::move(deadline_start_result.value())};

    deadline_handle.stop();
}

TEST_F(DeadlineFixture, Start_AlreadyRunning)
{
    // Get deadline.
    DeadlineTag deadline_tag{"deadline"};
    auto get_deadline_result{deadline_monitor_->get_deadline(deadline_tag)};
    ASSERT_TRUE(get_deadline_result.has_value());
    auto deadline{std::move(get_deadline_result.value())};

    // Try to start the deadline twice.
    deadline.start();
    auto deadline_start_result{deadline.start()};
    ASSERT_FALSE(deadline_start_result.has_value());
    ASSERT_EQ(deadline_start_result.error(), Error::Failed);
}

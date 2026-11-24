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
#ifndef TESTS_UTILS_FIXTURES_ACTIVATION_FIXTURE_HPP
#define TESTS_UTILS_FIXTURES_ACTIVATION_FIXTURE_HPP

#include "tests/utils/fixtures/control_client.hpp"
#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/launch_manager/common/constants.hpp>
#include <score/mw/lifecycle/ilm_control.hpp>

/// @brief Fixture for tests that need run target activation queue manipulation
class ActivationFixture : public ControlClientFixture
{
  protected:
    void WaitForActivation(
        const score::mw::lifecycle::RunTargetActivationSource& expected_source,
        const score::mw::lifecycle::RunTargetName& expected_run_target) const
    {
        pop_event(
            [&expected_source, &expected_run_target](
                score::mw::lifecycle::RunTargetActivationSource source, score::mw::lifecycle::RunTargetName target) {
                EXPECT_EQ(source, expected_source);
                EXPECT_EQ(target, expected_run_target);
            });
    }

    void FillActivationQueue(score::mw::lifecycle::ILmControl& client) const
    {
        for (int i = 0; i < static_cast<int>(score::mw::lifecycle::internal::kPendingActivationQueueCapacity); ++i)
        {
            const score::mw::lifecycle::RunTargetName run_target{(i % 2 == 0) ? "run_target_a" : "run_target_b"};
            const auto result = client.activate_run_target(run_target, false);
            ASSERT_TRUE(result.has_value()) << "Request " << i << ": " << result.error().Message();
        }
    }

    void BlockRunTargetActivation(score::mw::lifecycle::ILmControl& client) const
    {
        const auto result = client.activate_run_target("run_target_gated", true);
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(client));
        ASSERT_TRUE(wait_for_file(gatedStartedPath("gated")));
    }

    void UnblockRunTargetActivation() const
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("gated")));
    }
};

#endif  // TESTS_UTILS_FIXTURES_ACTIVATION_FIXTURE_HPP

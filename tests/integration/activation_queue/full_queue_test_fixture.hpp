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
#ifndef TESTS_INTEGRATION_ACTIVATION_QUEUE_FULL_QUEUE_TEST_FIXTURE_HPP
#define TESTS_INTEGRATION_ACTIVATION_QUEUE_FULL_QUEUE_TEST_FIXTURE_HPP

#include "activation_queue_test_fixture.hpp"
#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <gtest/gtest.h>
#include <score/mw/launch_manager/common/constants.hpp>

/// @brief Fixture for tests of a full run target activation request queue.
class ActivationQueueFullTest : public ActivationQueueTest
{
  protected:
    void SetUp() override
    {
        ASSERT_TRUE(check_clean({gatedStartedPath("gated"), gatedReleasePath("gated")}));
        InitializeClient();

        TEST_STEP("Wait for the initial run target to start")
        {
            WaitForActivation(score::mw::lifecycle::RunTargetActivationSource::kInitialActivation, "Startup");
        }

        TEST_STEP("Block run target activation by starting gated process")
        {
            BlockRunTargetActivation();
        }

        TEST_STEP("Fill the run target activation request queue")
        {
            FillActivationQueue();
        }
    }

    void TearDown() override
    {
        UnblockRunTargetActivation();
    }
};

#endif  // TESTS_INTEGRATION_ACTIVATION_QUEUE_FULL_QUEUE_TEST_FIXTURE_HPP

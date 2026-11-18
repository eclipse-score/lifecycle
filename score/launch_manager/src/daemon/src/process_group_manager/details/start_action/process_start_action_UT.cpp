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

#include "score/mw/launch_manager/process_group_manager/details/mock_component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/mock_safe_process_map_inserter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/process_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

constexpr osal::ProcessID kPid = 42;

class ProcessStartActionTest : public Test
{
  protected:
    /// @brief Expect the process to be started with the configuration and get kPid.
    void expectProcessStarted()
    {
        EXPECT_CALL(launcher_, startProcess(_, _, Ref(config_)))
            .WillOnce(DoAll(SetArgReferee<0>(kPid), Return(osal::OsalReturnType::kSuccess)));
    }

    StrictMock<osal::MockIProcess> launcher_;
    StrictMock<MockSafeProcessMapInserter> process_map_;
    const configuration::ComponentConfig config_{};
    MockComponent component_;
    const ProcessStartAction sut_{launcher_, process_map_, config_};
};

TEST_F(ProcessStartActionTest, StartStartsProcessAndTracksItForComponent)
{
    expectProcessStarted();
    EXPECT_CALL(process_map_, insertIfNotTerminated(kPid, &component_))
        .WillOnce(Return(SafeProcessMapInsertResult{SafeProcessMapReturnType::kInserted}));

    const Result<StartedResource> result = sut_.start(cpp::stop_token{}, component_);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<ProcessHandle>(result.value().handle).pid, kPid);
    EXPECT_FALSE(result.value().exit_status.has_value());
}

TEST_F(ProcessStartActionTest, StartReturnsExitStatusOfAlreadyTerminatedProcess)
{
    expectProcessStarted();
    EXPECT_CALL(process_map_, insertIfNotTerminated(kPid, &component_))
        .WillOnce(Return(SafeProcessMapInsertResult{SafeProcessMapReturnType::kMatched, 7}));

    const Result<StartedResource> result = sut_.start(cpp::stop_token{}, component_);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<ProcessHandle>(result.value().handle).pid, kPid);
    ASSERT_TRUE(result.value().exit_status.has_value());
    EXPECT_EQ(result.value().exit_status.value(), 7);
}

TEST_F(ProcessStartActionTest, StartFailsWithoutTrackingWhenProcessCannotBeStarted)
{
    EXPECT_CALL(launcher_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kFail));
    // The strict process map fails the test if the process is tracked.

    const Result<StartedResource> result = sut_.start(cpp::stop_token{}, component_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

class ProcessStartActionTrackingErrorTest : public ProcessStartActionTest,
                                            public WithParamInterface<SafeProcessMapReturnType>
{
};

TEST_P(ProcessStartActionTrackingErrorTest, StartKillsProcessAndFailsWhenProcessCannotBeTracked)
{
    expectProcessStarted();
    EXPECT_CALL(process_map_, insertIfNotTerminated(kPid, &component_))
        .WillOnce(Return(SafeProcessMapInsertResult{GetParam()}));
    EXPECT_CALL(launcher_, forceTermination(kPid)).WillOnce(Return(osal::OsalReturnType::kSuccess));

    const Result<StartedResource> result = sut_.start(cpp::stop_token{}, component_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

INSTANTIATE_TEST_SUITE_P(
    AllTrackingErrors,
    ProcessStartActionTrackingErrorTest,
    Values(SafeProcessMapReturnType::kInsertionError, SafeProcessMapReturnType::kInvalidIdError));

}  // namespace
}  // namespace score::mw::lifecycle::internal

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

TEST(ProcessStartActionTest, StartCallsStartProcessWithConfig)
{
    StrictMock<osal::MockIProcess> launcher;
    const configuration::ComponentConfig config{};
    const ProcessStartAction process_start_action(launcher, config);

    EXPECT_CALL(launcher, startProcess(_, _, Ref(config))).WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(process_start_action.start(cpp::stop_token{}));
}

TEST(ProcessStartActionTest, StartSucceedsReturnsProcessHandle)
{
    NiceMock<osal::MockIProcess> launcher;
    const configuration::ComponentConfig config{};
    const ProcessStartAction process_start_action(launcher, config);
    const osal::ProcessID expected_pid = 42;

    ON_CALL(launcher, startProcess(_, _, _))
        .WillByDefault(DoAll(SetArgReferee<0>(expected_pid), Return(osal::OsalReturnType::kSuccess)));

    const Result<Handle> result = process_start_action.start(cpp::stop_token{});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(std::get<ProcessHandle>(result.value()).pid, expected_pid);
}

TEST(ProcessStartActionTest, StartFailsReturnsError)
{
    NiceMock<osal::MockIProcess> launcher;
    const configuration::ComponentConfig config{};
    const ProcessStartAction process_start_action(launcher, config);

    ON_CALL(launcher, startProcess(_, _, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<Handle> result = process_start_action.start(cpp::stop_token{});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

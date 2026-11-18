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
#include <array>
#include <cstddef>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

constexpr osal::ProcessID kPid = 42;

alignas(osal::IpcCommsSync) std::array<std::byte, sizeof(osal::IpcCommsSync)> sync_buffer;

// The semaphores are never touched, since the launcher is mocked.
const osal::IpcCommsP kSync{reinterpret_cast<osal::IpcCommsSync*>(sync_buffer.data()), [](osal::IpcCommsSync*) {
                            }};

class ProcessStartActionTest : public Test
{
  protected:
    /// @brief Expect the process to be started with the configuration and get kPid and kSync.
    void expectProcessStarted()
    {
        EXPECT_CALL(launcher_, startProcess(_, _, Ref(config_)))
            .WillOnce(DoAll(SetArgReferee<0>(kPid), SetArgReferee<1>(kSync), Return(osal::OsalReturnType::kSuccess)));
    }

    /// @brief Expect the handle to refer to the process started by expectProcessStarted().
    static void expectStartedProcessHandle(const Handle& handle)
    {
        const ProcessHandle* const process = std::get_if<ProcessHandle>(&handle);
        ASSERT_NE(process, nullptr);
        EXPECT_EQ(process->pid, kPid);
        EXPECT_EQ(process->sync, kSync);
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
    expectStartedProcessHandle(result.value().handle);
    EXPECT_FALSE(result.value().exit_status.has_value());
}

TEST_F(ProcessStartActionTest, StartReturnsExitStatusOfAlreadyTerminatedProcess)
{
    expectProcessStarted();
    EXPECT_CALL(process_map_, insertIfNotTerminated(kPid, &component_))
        .WillOnce(Return(SafeProcessMapInsertResult{SafeProcessMapReturnType::kMatched, 7}));

    const Result<StartedResource> result = sut_.start(cpp::stop_token{}, component_);

    ASSERT_TRUE(result.has_value());
    expectStartedProcessHandle(result.value().handle);
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

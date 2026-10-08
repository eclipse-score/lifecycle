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

#include "score/mw/lifecycle/execution_error.h"
#include "score/mw/lifecycle/lifecycle_client/details/mock_report_running_impl.h"
#include "score/mw/lifecycle/report_running.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

namespace score::mw::lifecycle
{

// Access to the reported flag reset function
extern std::atomic_bool& GetReportedFlag();

/// @brief A test fixture for report running tests
/// @note ResetReportedFlag is used to restore the static state of the static g_impl between test runs.
class ReportRunningTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Common test properties
        RecordProperty("TestType", "interface-test");

        // Reset the static reported flag before each test
        mock_.ResetReportedFlag();

        // Set up default expectations for constructor/destructor
        EXPECT_CALL(mock_, ctor()).Times(::testing::AnyNumber());
        EXPECT_CALL(mock_, dtor()).Times(::testing::AnyNumber());
    }

    void TearDown() override
    {
        // Clean up the reported flag after each test
        mock_.ResetReportedFlag();
    }

    ReportRunningImplMock mock_;
};

TEST_F(ReportRunningTest, GivenAReportingProcess_ExpectReportRunningImplCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a reporting process. "
        "Expect ReportRunningImpl{}.ReportRunningState() is called. "
        "When report_running() is called. "
        "Then no exceptions are thrown.");

    // Expect
    EXPECT_CALL(mock_, ReportRunningState()).WillOnce(Return(score::Result<std::monostate>{}));

    // When, Then
    EXPECT_NO_THROW(report_running());
}

TEST_F(ReportRunningTest, GivenAReportingProcessUsingCApi_ExpectReportRunningImplCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a reporting process using the C API. "
        "Expect ReportRunningImpl{}.ReportRunningState() to be called. "
        "When score_mw_lifecycle_report_running() is called. "
        "Then score_mw_lifecycle_report_running() succeeds.");

    // Expect
    EXPECT_CALL(mock_, ReportRunningState()).WillOnce(Return(score::Result<std::monostate>{}));

    // When
    const int8_t result = score_mw_lifecycle_report_running();

    // Then
    EXPECT_EQ(result, 0);
}

TEST_F(
    ReportRunningTest,
    GivenAReportingProcessUsingCApi_ExpectSubsequentCallsToFail_WhenReportRunningCalledMultipleTimes)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a reporting process using a C API. "
        "Expect first report_running() call to pass, and subsequent calls to fail. "
        "When score_mw_lifecycle_report_running() is called multiple times. "
        "Then score_mw_lifecycle_report_running() should succeed on first call, "
        "and return -1 on subsequent calls.");

    // Expect
    EXPECT_CALL(mock_, ReportRunningState())
        .WillOnce(Return(score::Result<std::monostate>{}))
        .WillOnce(Return(score::Result<std::monostate>{score::MakeUnexpected(ExecErrc::kInvalidTransition)}));

    // When
    auto first_call_result = score_mw_lifecycle_report_running();
    auto subsequent_call_result = score_mw_lifecycle_report_running();

    // Then
    EXPECT_EQ(first_call_result, 0);
    EXPECT_EQ(subsequent_call_result, -1);
}

/// @brief A test fixture for a parameterized test taking an ExecErrc value
class ReportRunningErrorTest : public ReportRunningTest, public testing::WithParamInterface<ExecErrc>
{
};

TEST_P(ReportRunningErrorTest, GivenAReportingProcessUsingCApiWithError_ExpectReportRunningImplFails)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");

    // Given a reporting process using C Api, with a communication error
    const auto ERROR = ExecErrc::kCommunicationError;

    // Expect ReportRunningImpl{}.ReportRunningState() fails
    EXPECT_CALL(mock_, ReportRunningState())
        .WillOnce(Return(score::Result<std::monostate>{score::MakeUnexpected(ERROR)}));

    // When score_mw_lifecycle_report_running() is called
    const int8_t result = score_mw_lifecycle_report_running();

    // Then score_mw_lifecycle_report_running() should return -1
    EXPECT_EQ(result, -1);
}

TEST_P(ReportRunningErrorTest, GivenACommunicationError_ExpectReportRunningImplFails)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given a communication error. "
        "Expect ReportRunningImpl{}.ReportRunningState() to fail. "
        "When report_running() is called. "
        "Then no exceptions are thrown.");

    // Given
    const auto ERROR = GetParam();

    // Expect
    EXPECT_CALL(mock_, ReportRunningState())
        .WillOnce(Return(score::Result<std::monostate>{score::MakeUnexpected(ERROR)}));

    // When, Then
    EXPECT_NO_THROW(report_running());
}

// All possible error codes are in the same equivalence class, where they cause score_mw_lifecycle_report_running to
// return -1 We have only included a handful of the possible errors from the equivalence class Strictly speaking we only
// need to test one value from the equivalence class
INSTANTIATE_TEST_SUITE_P(
    FailureEquivalenceClass,
    ReportRunningErrorTest,
    testing::Values(ExecErrc::kGeneralError, ExecErrc::kCommunicationError, ExecErrc::kInvalidTransition));

}  // namespace score::mw::lifecycle

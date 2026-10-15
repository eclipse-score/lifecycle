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

#include "applicationcontext.h"
#include "score/mw/lifecycle/application.h"
#include "score/mw/lifecycle/lifecycle_client/details/mock_report_running_impl.h"
#include "score/mw/lifecycle/lifecyclemanager.h"
#include "score/mw/lifecycle/mock_application.h"
#include "score/os/errno.h"
#include "score/os/mocklib/stdlib_mock.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "tests/utils/test_helper/one_shot_event.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <optional>
#include <thread>

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Return;

namespace
{

using SigResult = score::cpp::expected<std::int32_t, score::os::Error>;

// Test fixture
class LifeCycleManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        // Set up default expectations for report_running mock
        EXPECT_CALL(report_running_mock_, ctor()).Times(AnyNumber());
        EXPECT_CALL(report_running_mock_, dtor()).Times(AnyNumber());
        EXPECT_CALL(report_running_mock_, ReportRunningState())
            .Times(AnyNumber())
            .WillRepeatedly(Return(score::Result<std::monostate>{}));
    }

    void TearDown() override
    {
        if (stdlib_mock_injected_)
        {
            score::os::Stdlib::restore_instance();
        }
        report_running_mock_.ResetReportedFlag();
    }

    /// @brief Installs the stdlib mock as testing instance.
    /// It is restored in TearDown
    score::os::StdlibMock& InstallStdlibMock()
    {
        score::os::Stdlib::set_testing_instance(stdlib_mock_);
        stdlib_mock_injected_ = true;
        return stdlib_mock_;
    }

    /// @brief Mock stdlib functions
    score::os::StdlibMock stdlib_mock_;

    // @brief True if stdlib mock is injected, false otherwise
    bool stdlib_mock_injected_{false};

    /// @brief Mock ReportRunningImpl class
    score::mw::lifecycle::ReportRunningImplMock report_running_mock_;

    /// @brief Released by SendSelfSigterm, which unblocks the mocked SigWait
    OneShotEvent sigterm_sent_;

    /// @brief Helper function to set expectations so that SigWait() will wait until SendSelfSigterm() is called.
    void ExpectSigWaitUntilSigterm(
        score::os::SignalMock& signal_mock,
        const SigResult& wait_result,
        const SigResult& send_result)
    {
        EXPECT_CALL(signal_mock, SigWait(_, _))
            .WillOnce(Invoke([this, wait_result](const sigset_t&, std::int32_t& sig) {
                sigterm_sent_.Wait();
                sig = SIGTERM;
                return wait_result;
            }));

        EXPECT_CALL(signal_mock, SendSelfSigterm()).WillOnce(Invoke([this, send_result]() {
            sigterm_sent_.Signal();
            return send_result;
        }));
    }

    /// @brief Helper function to create a signal mock with successful initialization expectations
    std::unique_ptr<score::os::SignalMock> CreateSuccessfulSignalMock()
    {
        auto signal_mock = std::make_unique<score::os::SignalMock>();
        EXPECT_CALL(*signal_mock, SigEmptySet(_))
            .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
        EXPECT_CALL(*signal_mock, AddTerminationSignal(_))
            .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
        EXPECT_CALL(*signal_mock, PthreadSigMask(_))
            .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
        return signal_mock;
    }
};

TEST_F(
    LifeCycleManagerTest,
    GivenValidApplication_ExpectSigWaitUntilTerm_WhenRunCalledWithSucceedingInitAndRun_ThenReturnsZero)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with valid application and context. "
        "Expect SigWait to wait until SIGTERM is received. "
        "Expect Application::Initialize() to be called once, and return 0. "
        "Expect Application::Run() to be called once, and return 0. "
        "When run() is called on the LifeCycleManager. "
        "Then run() returns 0.");

    // Given
    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(1, argv);

    // Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{0}, score::cpp::expected<std::int32_t, score::os::Error>{0});
    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
    EXPECT_CALL(app, Run(_)).WillOnce(Return(0));

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 0);
}

TEST_F(
    LifeCycleManagerTest,
    GivenAppFailingInitialize_ExpectInitErrorCodeAndRunNotCalled_WhenRunCalled_ThenReturnsErrorCode)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with application that fails Initialize, and a valid context. "
        "Expect SigWait to wait until SIGTERM is received. "
        "Expect the configured application to fail Initialize() with a 42 return code. "
        "Expect the configured application Run() call not to be made. "
        "When run() is called. "
        "Then run() returns the Initialize error code.");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{0}, score::cpp::expected<std::int32_t, score::os::Error>{0});
    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(42));
    EXPECT_CALL(app, Run(_)).Times(0);
    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(1, argv);

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 42);
}

TEST_F(LifeCycleManagerTest, GivenAppFailingRun_ExpectRunErrorCode_WhenRunCalled_ThenReturnsErrorCode)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with application that succeeds Initialize but fails Run. "
        "Expect SigWait to wait until SIGTERM is received. "
        "Expect the configured application to succeed Initialize() with a 0 return code. "
        "Expect the configured application to fail Run() call with error code 99. "
        "When run() is called. "
        "Then run() returns the application Run error code.");

    // Given
    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(1, argv);

    // Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{0}, score::cpp::expected<std::int32_t, score::os::Error>{0});
    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
    EXPECT_CALL(app, Run(_)).WillOnce(Return(99));

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 99);
}

TEST_F(LifeCycleManagerTest, GivenAppContextWithArguments_ExpectAppCallSuccess_WhenRunCalled_ThenRunsSuccessfully)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager and ApplicationContext with arguments. "
        "Expect Application::Initialize() to be called once, returning 0. "
        "Expect Application::Run() to be called once, returning 0. "
        "When run() is called. "
        "Then run() returns 0.");

    // Given, Expect
    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(1, argv);
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{0}, score::cpp::expected<std::int32_t, score::os::Error>{0});
    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
    EXPECT_CALL(app, Run(_)).WillOnce(Return(0));

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 0);
}

TEST_F(LifeCycleManagerTest, GivenRunningSignalHandlerThread_ExpectSignalHandled_WhenDestructorCalled_ThenExitsCleanly)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with running signal handler thread and no application set. "
        "Expect SigWait() to be called once by LifeCycleManager::handle_signal(), and to return SIGTERM. "
        "Expect std::quick_exit(EXIT_FAILURE) to be called once by handle_signal(), as no application is set. "
        "Expect SendSelfSigterm() to be called once by LifeCycleManager::at_exit(), "
        "only after the signal handler is blocked in SigWait(). "
        "When destructor is called. "
        "Then the signal handler thread is joined (the destructor returns).");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    OneShotEvent sigwait_entered;

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, quick_exit(EXIT_FAILURE)).Times(1);

    EXPECT_CALL(*signal_mock, SigWait(_, _))
        .WillOnce(Invoke([this, &sigwait_entered](const sigset_t&, std::int32_t& sig) {
            sigwait_entered.Signal();
            sigterm_sent_.Wait();
            sig = SIGTERM;
            return SigResult{0};
        }));

    EXPECT_CALL(*signal_mock, SendSelfSigterm()).WillOnce(Invoke([this]() {
        sigterm_sent_.Signal();
        return SigResult{0};
    }));

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
        sigwait_entered.Wait();
    }  // Destructor called here

    // Then
    // The destructor blocks on join(), so returning here means the thread was joined
}

TEST_F(LifeCycleManagerTest, GivenLifeCycleManager_ExpectSigEmptySetFailure_WhenLifecycleManagerConstructed_ThenExits)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a signal interface. "
        "Expect SigEmptySet() to fail. "
        "Expect AddTerminationSignal(), PthreadSigMask(), SigWait() and SendSelfSigterm() not to be called. "
        "Expect exit(EXIT_FAILURE) to be called. "
        "When LifeCycleManager is constructed. "
        "Then no signal handler thread is started and exit(EXIT_FAILURE) is called.");

    // Given, Expect
    auto signal_mock = std::make_unique<score::os::SignalMock>();
    EXPECT_CALL(*signal_mock, SigEmptySet(_))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_CALL(*signal_mock, AddTerminationSignal(_)).Times(0);
    EXPECT_CALL(*signal_mock, PthreadSigMask(_)).Times(0);
    EXPECT_CALL(*signal_mock, SigWait(_, _)).Times(0);
    EXPECT_CALL(*signal_mock, SendSelfSigterm()).Times(0);

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, exit(EXIT_FAILURE)).Times(1);

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    }

    // Then
    // exit(EXIT_FAILURE) was called exactly once, verified by the mock expectation
}

TEST_F(
    LifeCycleManagerTest,
    GivenLifecycleManager_ExpectAddTerminationSignalFailure_WhenLifecycleManagerConstructed_ThenExits)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a signal interface. "
        "Expect SigEmptySet() to succeed. "
        "Expect AddTerminationSignal() to fail. "
        "Expect PthreadSigMask(), SigWait() and SendSelfSigterm() not to be called. "
        "Expect exit(EXIT_FAILURE) to be called. "
        "When LifeCycleManager is constructed. "
        "Then no signal handler thread is started and exit(EXIT_FAILURE) is called.");

    // Given, Expect
    auto signal_mock = std::make_unique<score::os::SignalMock>();
    EXPECT_CALL(*signal_mock, SigEmptySet(_)).WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
    EXPECT_CALL(*signal_mock, AddTerminationSignal(_))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_CALL(*signal_mock, PthreadSigMask(_)).Times(0);
    EXPECT_CALL(*signal_mock, SigWait(_, _)).Times(0);
    EXPECT_CALL(*signal_mock, SendSelfSigterm()).Times(0);

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, exit(EXIT_FAILURE)).Times(1);

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    }

    // Then
    // exit(EXIT_FAILURE) was called exactly once, verified by the mock expectation
}

TEST_F(
    LifeCycleManagerTest,
    GivenLifecycleManager_ExpectPthreadSigMaskFailure_WhenLifecycleManagerConstructed_ThenExits)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a signal interface. "
        "Expect SigEmptySet() and AddTerminationSignal() to succeed. "
        "Expect PthreadSigMask() to fail. "
        "Expect SigWait() and SendSelfSigterm() not to be called. "
        "Expect exit(EXIT_FAILURE) to be called. "
        "When LifeCycleManager is constructed. "
        "Then no signal handler thread is started and exit(EXIT_FAILURE) is called.");

    // Given, Expect
    auto signal_mock = std::make_unique<score::os::SignalMock>();
    EXPECT_CALL(*signal_mock, SigEmptySet(_)).WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
    EXPECT_CALL(*signal_mock, AddTerminationSignal(_))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));
    EXPECT_CALL(*signal_mock, PthreadSigMask(_))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_CALL(*signal_mock, SigWait(_, _)).Times(0);
    EXPECT_CALL(*signal_mock, SendSelfSigterm()).Times(0);

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, exit(EXIT_FAILURE)).Times(1);

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    }

    // Then
    // exit(EXIT_FAILURE) was called exactly once, verified by the mock expectation
}

TEST_F(LifeCycleManagerTest, GivenRunningApplication_ExpectStopNotRequested_WhenSigWaitFails_ThenExitsWithFailure)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager running an application. "
        "Expect SigWait() to fail with EINTR once SendSelfSigterm() is called by at_exit(). "
        "Expect Application::Initialize() and Run() to be called once, each returning 0. "
        "Expect std::quick_exit(EXIT_FAILURE) to be called once by handle_signal(). "
        "When SigWait() fails. "
        "Then stop is not requested on the application's stop token, and run() returns 0.");

    std::optional<score::cpp::stop_token> captured_token;
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(
        *signal_mock, SigResult{score::cpp::make_unexpected(score::os::Error::createFromErrno(EINTR))}, SigResult{0});

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, quick_exit(EXIT_FAILURE)).Times(1);

    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
        score::mw::lifecycle::MockApplication app;
        EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
        EXPECT_CALL(app, Run(_)).WillOnce(Invoke([&captured_token](const score::cpp::stop_token& token) {
            captured_token = token;
            return 0;
        }));

        const char* argv[] = {"test_app"};
        score::mw::lifecycle::ApplicationContext context(1, argv);
        EXPECT_EQ(manager.run(app, context), 0);
    }

    // Then
    // The handler thread has been joined, so the stop state is final
    ASSERT_TRUE(captured_token.has_value());
    EXPECT_FALSE(captured_token->stop_requested());
}

TEST_F(LifeCycleManagerTest, GivenNoApplication_ExpectQuickExitCalled_WhenSigWaitReturnsNonZero_ThenExitsWithFailure)
{
    RecordProperty("DerivationTechnique", "boundary-value-analysis");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with no application set. "
        "Expect SigWait() to succeed but return a non-zero value, once SendSelfSigterm() is called by at_exit(). "
        "Expect std::quick_exit(EXIT_FAILURE) to be called once by handle_signal(). "
        "When the LifeCycleManager is destroyed. "
        "Then quick_exit(EXIT_FAILURE) is called and the thread is joined.");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{1}, score::cpp::expected<std::int32_t, score::os::Error>{0});

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, quick_exit(EXIT_FAILURE)).Times(1);

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    }

    // Then
    // quick_exit(EXIT_FAILURE) was called exactly once, verified by the mock expectation
}

TEST_F(
    LifeCycleManagerTest,
    GivenRunningSignalHandlerThread_ExpectThreadJoined_WhenSendSelfSigtermFailsInDestructor_ThenExitsCleanly)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager with running signal handler thread and no application set. "
        "Expect SendSelfSigterm() to be called once by at_exit() and to fail with EPERM. "
        "Expect SigWait() to block until SendSelfSigterm() is called, then return SIGTERM. "
        "Expect std::quick_exit(EXIT_FAILURE) to be called once by handle_signal(), as no application is set. "
        "When LifeCycleManager destructor is called. "
        "Then the failure is tolerated and the thread is still joined.");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(
        *signal_mock, SigResult{0}, score::cpp::make_unexpected(score::os::Error::createFromErrno(EPERM)));

    auto& stdlib_mock = InstallStdlibMock();
    EXPECT_CALL(stdlib_mock, quick_exit(EXIT_FAILURE)).Times(1);

    // When
    {
        score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    }

    // Then
    // The destructor blocks on join(), so returning here means the thread was joined
}

TEST_F(LifeCycleManagerTest, GivenRunningApplication_ExpectStopRequested_WhenSigtermReceived_ThenStopsApplication)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager running an application. "
        "Expect Application::Initialize() to be called once, returning 0. "
        "Expect Application::Run() to be called once, to loop until stop is requested, then return 0. "
        "Expect SigWait() to return SIGTERM once Run() has started. "
        "Expect SendSelfSigterm() to be called once by at_exit() after Run() returns. "
        "When SIGTERM is received. "
        "Then stop is requested on the application's stop token and run() returns 0.");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    OneShotEvent app_run_started;
    std::atomic<bool> stop_requested{false};

    EXPECT_CALL(*signal_mock, SigWait(_, _)).WillOnce(Invoke([&app_run_started](const sigset_t&, std::int32_t& sig) {
        app_run_started.Wait();
        sig = SIGTERM;
        return score::cpp::expected<std::int32_t, score::os::Error>{0};
    }));
    EXPECT_CALL(*signal_mock, SendSelfSigterm())
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{0}));

    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
    EXPECT_CALL(app, Run(_)).WillOnce(Invoke([&app_run_started, &stop_requested](const score::cpp::stop_token& token) {
        app_run_started.Signal();
        // Bounded wait so a missing stop request fails the test instead of hanging it
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!token.stop_requested() && (std::chrono::steady_clock::now() < deadline))
        {
            std::this_thread::yield();
        }
        stop_requested.store(token.stop_requested());
        return 0;
    }));

    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(1, argv);

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(stop_requested.load());
}

TEST_F(
    LifeCycleManagerTest,
    GivenEmptyApplicationContext_ExpectSuccessfulRunWithoutArguments_WhenRunCalled_ThenReturnsZero)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a LifeCycleManager and an ApplicationContext with no arguments (argc == 0). "
        "Expect Application::Initialize() to be called once, returning 0. "
        "Expect Application::Run() to be called once, returning 0. "
        "When run() is called. "
        "Then run() returns 0.");

    // Given, Expect
    auto signal_mock = CreateSuccessfulSignalMock();
    ExpectSigWaitUntilSigterm(*signal_mock, SigResult{0}, score::cpp::expected<std::int32_t, score::os::Error>{0});
    score::mw::lifecycle::LifeCycleManager manager(std::move(signal_mock));
    score::mw::lifecycle::MockApplication app;
    EXPECT_CALL(app, Initialize(_)).WillOnce(Return(0));
    EXPECT_CALL(app, Run(_)).WillOnce(Return(0));

    // Although we pass argc == 0, we have one element in argv.
    // the ApplicationContext constructor unconditionally reads argv[0] to initialise the application path, so argv must
    // not be empty or null.
    const char* argv[] = {"test_app"};
    score::mw::lifecycle::ApplicationContext context(0, argv);
    ASSERT_TRUE(context.get_arguments().empty());

    // When
    const auto result = manager.run(app, context);

    // Then
    EXPECT_EQ(result, 0);
}

}  // namespace

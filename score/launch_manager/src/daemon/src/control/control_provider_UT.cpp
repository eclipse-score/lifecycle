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

#include "score/mw/launch_manager/control/control_provider.hpp"
#include "score/mw/launch_manager/process_group_manager/irun_target_control.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdarg>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

// ============================================================================
// fcntl() linker wrapper — offerService FD_CLOEXEC workaround test support
//
// These declarations must live at file scope: extern "C" gives functions
// external C linkage, which is incompatible with an anonymous namespace.
//
// Activated via --wrap=fcntl in the BUILD target's linkopts. The linker
// replaces every call to fcntl() with __wrap_fcntl(), and makes the original
// implementation available as __real_fcntl().
//
// Only calls that match the currently injected (fd, cmd) pair are intercepted;
// all other calls are forwarded to __real_fcntl() transparently.
//
// ============================================================================
//
// Why would we like to have a mixed setup where sometimes we use a real
// implementation and sometimes we use wrapper?
//
// The OfferService_OpenFd_SetsCloexec test checks that code correctly set the
// FD_CLOEXEC on a real file descriptor created by OS.
//
// The OfferService_FSetfdFails_LogsAndContinues tests a corner case where
// fcntl(fd, F_GETFD) succeed, but fcntl(fd, F_SETFD, flags) fails.
// To inject this type of the failure without a wrapper, we would need to
// close an fd after fcntl(fd, F_GETFD), but before fcntl(fd, F_SETFD, flags).
//
// This can be done with a bit of multi threading, but it would be flaky...
// Wrapper is more stable.
//
// And yes, the OfferService_OpenFd_SetsCloexec could also go through wrapper,
// but current implementation is much more close to the real life scenario.
// ============================================================================

/// @brief Controls whether and how a single fcntl() call is intercepted.
struct FcntlInjection
{
    bool active{false};
    int target_fd{-1};
    int target_cmd{-1};
    int errno_value{EINVAL};
};

/// @brief Global injection state — written by ScopedFcntlError, read by __wrap_fcntl.
static FcntlInjection g_fcntl_injection{};

/// @brief The original fcntl() implementation, renamed by the linker.
extern "C" int __real_fcntl(int fd, int cmd, ...);

/// @brief Intercepts every fcntl() call in the test binary.
///        Injects a configurable error for the targeted (fd, cmd) pair;
///        forwards everything else to the real implementation unchanged.
extern "C" int __wrap_fcntl(int fd, int cmd, ...)
{
    // Extract the optional third argument. fcntl() accepts either an int
    // (F_SETFD, F_SETFL, …) or a pointer (F_GETLK, …). Extracting as int
    // and forwarding is safe for the integer commands used in offerService();
    // non-integer commands are never intercepted so the forwarded value is
    // ignored by __real_fcntl's own va_arg handling.
    va_list args;
    va_start(args, cmd);
    const int third_arg = va_arg(args, int);
    va_end(args);

    if (g_fcntl_injection.active && fd == g_fcntl_injection.target_fd && cmd == g_fcntl_injection.target_cmd)
    {
        // error injections through the wrapper
        errno = g_fcntl_injection.errno_value;
        return -1;
    }

    // forwarding to the real implementation
    return __real_fcntl(fd, cmd, third_arg);
}

namespace score::mw::lifecycle::internal
{
namespace
{

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

// ============================================================================
// Fake infrastructure
// ============================================================================

/// @brief Minimal value type that mirrors the SampleAllocateePtr<ActivationResult> interface
///        used by the real skeleton event. Copyable so SaveArg works without complications.
struct FakeAllocateePtr
{
    ActivationResult data{};
    ActivationResult* Get() noexcept
    {
        return &data;
    }
};

/// @brief Single gmock class that all fake skeleton operations are routed to.
class SkeletonMock
{
  public:
    MOCK_METHOD(score::Result<void>, SkeletonCreate, ());
    MOCK_METHOD(score::Result<void>, OfferService, ());
    MOCK_METHOD(
        score::Result<void>,
        ActivateRunTargetRegisterHandler,
        (std::function<void(ActivateRunTargetResponse&, const ActivateRunTargetRequest&)>));
    MOCK_METHOD(
        score::Result<void>,
        GetActiveRunTargetRegisterHandler,
        (std::function<void(GetActiveRunTargetResponse&)>));
    MOCK_METHOD(score::Result<FakeAllocateePtr>, Allocate, ());
    MOCK_METHOD(score::Result<void>, Send, (FakeAllocateePtr));
};

/// @brief Adapter for skeleton_.activate_run_target — delegates RegisterHandler to SkeletonMock.
struct FakeActivateMethod
{
    SkeletonMock* mock;

    template <typename Callable>
    score::Result<void> RegisterHandler(Callable&& cb)
    {
        return mock->ActivateRunTargetRegisterHandler(
            std::function<void(ActivateRunTargetResponse&, const ActivateRunTargetRequest&)>(
                std::forward<Callable>(cb)));
    }
};

/// @brief Adapter for skeleton_.get_active_run_target — delegates RegisterHandler to SkeletonMock.
struct FakeGetActiveMethod
{
    SkeletonMock* mock;

    template <typename Callable>
    score::Result<void> RegisterHandler(Callable&& cb)
    {
        return mock->GetActiveRunTargetRegisterHandler(
            std::function<void(GetActiveRunTargetResponse&)>(std::forward<Callable>(cb)));
    }
};

/// @brief Adapter for skeleton_.activation_result — delegates Allocate/Send to SkeletonMock.
struct FakeActivationResultEvent
{
    SkeletonMock* mock;

    score::Result<FakeAllocateePtr> Allocate()
    {
        return mock->Allocate();
    }
    score::Result<void> Send(FakeAllocateePtr p)
    {
        return mock->Send(std::move(p));
    }
};

/// @brief Drop-in replacement for LmControlSkeleton. Holds the three adapter members
///        and routes Create()/OfferService() through the static mock pointer.
struct FakeLmControlSkeleton
{
    static inline SkeletonMock* mock{nullptr};

    FakeActivateMethod activate_run_target;
    FakeGetActiveMethod get_active_run_target;
    FakeActivationResultEvent activation_result;

    FakeLmControlSkeleton() : activate_run_target{mock}, get_active_run_target{mock}, activation_result{mock}
    {
    }

    static score::Result<FakeLmControlSkeleton> Create(const com::InstanceSpecifier&)
    {
        const auto result = mock->SkeletonCreate();
        if (!result.has_value())
        {
            return MakeUnexpected(ExecErrc::kCommunicationError);
        }
        return FakeLmControlSkeleton{};
    }

    score::Result<void> OfferService()
    {
        return mock->OfferService();
    }
};

/// @brief Specifier factory whose failure mode can be toggled per test.
struct FakeSpecifierFactory
{
    static inline bool should_fail{false};

    static score::Result<com::InstanceSpecifier> Create()
    {
        if (should_fail)
        {
            return MakeUnexpected(ExecErrc::kCommunicationError);
        }
        return com::InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    }
};

/// @brief gmock implementation of IRunTargetControl.
class MockRunTargetControl : public IRunTargetControl
{
  public:
    MOCK_METHOD(score::Result<IdentifierHash>, getActiveRunTarget, (), (const, noexcept, override));
    MOCK_METHOD(score::Result<void>, setRequestedRunTarget, (IdentifierHash), (noexcept, override));
    MOCK_METHOD(void, registerActiveRunTargetCallback, (ActivationCallbackT), (noexcept, override));
};

using TestControlProvider = BasicControlProvider<FakeLmControlSkeleton, FakeSpecifierFactory>;

// ============================================================================
// Fixture
// ============================================================================

class ControlProviderUT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        FakeLmControlSkeleton::mock = &skeleton_mock_;
        FakeSpecifierFactory::should_fail = false;

        // Sensible defaults so tests only need to wire what they care about.
        ON_CALL(skeleton_mock_, SkeletonCreate()).WillByDefault(Return(score::Result<void>{}));
        ON_CALL(skeleton_mock_, ActivateRunTargetRegisterHandler(_))
            .WillByDefault(DoAll(SaveArg<0>(&on_activate_), Return(score::Result<void>{})));
        ON_CALL(skeleton_mock_, GetActiveRunTargetRegisterHandler(_))
            .WillByDefault(DoAll(SaveArg<0>(&on_get_active_), Return(score::Result<void>{})));
        ON_CALL(skeleton_mock_, OfferService()).WillByDefault(Return(score::Result<void>{}));
        ON_CALL(skeleton_mock_, Allocate()).WillByDefault(Return(score::Result<FakeAllocateePtr>{FakeAllocateePtr{}}));
        ON_CALL(skeleton_mock_, Send(_)).WillByDefault(Return(score::Result<void>{}));

        ON_CALL(graph_, registerActiveRunTargetCallback(_)).WillByDefault(Invoke([this](ActivationCallbackT cb) {
            activation_callback_ = std::move(cb);
        }));
        ON_CALL(graph_, getActiveRunTarget()).WillByDefault(Return(score::Result<IdentifierHash>{default_run_target_}));
        ON_CALL(graph_, setRequestedRunTarget(_)).WillByDefault(Return(score::Result<void>{}));
    }

    void TearDown() override
    {
        FakeLmControlSkeleton::mock = nullptr;
    }

    /// @brief Build a successfully-constructed provider using fixture defaults.
    std::unique_ptr<TestControlProvider> MakeProvider()
    {
        auto result = TestControlProvider::Create(&graph_);
        EXPECT_TRUE(result.has_value());
        return std::move(result).value();
    }

    /// @brief Fire the activation callback captured during Create(), as if the graph just
    ///        completed a Run Target transition.
    void TriggerActivation(IdentifierHash state, RunTargetActivationSource source)
    {
        ASSERT_TRUE(activation_callback_) << "registerActiveRunTargetCallback was never called";
        activation_callback_(state, source);
    }

    // Registered by IdentifierHash constructor — stays in the global registry for the
    // process lifetime. Use unique names per translation unit to avoid cross-test collisions.
    const IdentifierHash default_run_target_{"cp_ut_default"};

    NiceMock<SkeletonMock> skeleton_mock_;
    NiceMock<MockRunTargetControl> graph_;

    // Handlers captured via SaveArg during Create() / MakeProvider():
    std::function<void(ActivateRunTargetResponse&, const ActivateRunTargetRequest&)> on_activate_;
    std::function<void(GetActiveRunTargetResponse&)> on_get_active_;
    ActivationCallbackT activation_callback_;
};

// ============================================================================
// Static assertions — compile-time contract: no copy, no move.
// ============================================================================

TEST_F(ControlProviderUT, StaticAssertionsCompiled)
{
    RecordProperty(
        "Description",
        "BasicControlProvider is neither movable nor copyable. Callbacks capture `this` "
        "and the object must never relocate. A compile-time regression here would "
        "silently reintroduce a dangling-callback bug.");

    static_assert(!std::is_move_constructible_v<TestControlProvider>);
    static_assert(!std::is_move_assignable_v<TestControlProvider>);
    static_assert(!std::is_copy_constructible_v<TestControlProvider>);
    static_assert(!std::is_copy_assignable_v<TestControlProvider>);

    SUCCEED();
}

// ============================================================================
// Create() — failure branches
// ============================================================================

TEST_F(ControlProviderUT, Create_SpecifierCreationFails_ReturnsCommunicationError)
{
    RecordProperty(
        "Description",
        "When the specifier factory fails, Create() returns kCommunicationError "
        "without attempting skeleton creation.");

    FakeSpecifierFactory::should_fail = true;
    EXPECT_CALL(skeleton_mock_, SkeletonCreate()).Times(0);

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ControlProviderUT, Create_SkeletonCreationFails_ReturnsCommunicationError)
{
    RecordProperty(
        "Description",
        "When skeleton creation fails, Create() returns kCommunicationError "
        "without registering any handlers.");

    EXPECT_CALL(skeleton_mock_, SkeletonCreate()).WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(skeleton_mock_, ActivateRunTargetRegisterHandler(_)).Times(0);

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ControlProviderUT, Create_RegisterActivateHandlerFails_ReturnsCommunicationError)
{
    RecordProperty(
        "Description",
        "When registering the activate_run_target handler fails, Create() returns "
        "kCommunicationError without proceeding to the get_active_run_target handler.");

    EXPECT_CALL(skeleton_mock_, ActivateRunTargetRegisterHandler(_))
        .WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(skeleton_mock_, GetActiveRunTargetRegisterHandler(_)).Times(0);

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ControlProviderUT, Create_RegisterGetActiveHandlerFails_ReturnsCommunicationError)
{
    RecordProperty(
        "Description",
        "When registering the get_active_run_target handler fails, Create() returns "
        "kCommunicationError without offering the service.");

    EXPECT_CALL(skeleton_mock_, GetActiveRunTargetRegisterHandler(_))
        .WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(skeleton_mock_, OfferService()).Times(0);

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ControlProviderUT, Create_OfferServiceFails_ReturnsCommunicationError)
{
    RecordProperty("Description", "When OfferService fails, Create() returns kCommunicationError.");

    EXPECT_CALL(skeleton_mock_, OfferService()).WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ControlProviderUT, Create_Success_RegistersHandlersAndOffersService)
{
    RecordProperty(
        "Description",
        "On the happy path Create() registers both method handlers, registers the "
        "activation callback with the graph, offers the service, and returns a "
        "non-null provider.");

    EXPECT_CALL(skeleton_mock_, ActivateRunTargetRegisterHandler(_)).Times(1);
    EXPECT_CALL(skeleton_mock_, GetActiveRunTargetRegisterHandler(_)).Times(1);
    EXPECT_CALL(graph_, registerActiveRunTargetCallback(_)).Times(1);
    EXPECT_CALL(skeleton_mock_, OfferService()).Times(1);

    const auto result = TestControlProvider::Create(&graph_);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

// ============================================================================
// handleActivateRunTarget
// ============================================================================

TEST_F(ControlProviderUT, ActivateRunTarget_QueuedMode_Rejected)
{
    RecordProperty(
        "Description",
        "A queued-mode activation request is rejected with kRejected/kNotImplemented; "
        "the graph is never consulted.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_activate_) << "activate_run_target handler was not registered";

    EXPECT_CALL(graph_, setRequestedRunTarget(_)).Times(0);

    ActivateRunTargetResponse response{};
    const ActivateRunTargetRequest request{RunTargetName{"any_target"}, ActivationMode::kQueued};
    on_activate_(response, request);

    EXPECT_EQ(response.status, RequestStatus::kRejected);
    EXPECT_EQ(response.rejection_reason, ExecErrc::kNotImplemented);
}

TEST_F(ControlProviderUT, ActivateRunTarget_UnknownRunTarget_Rejected)
{
    RecordProperty(
        "Description",
        "A forced activation for a name absent from the IdentifierHash registry is "
        "rejected with kRejected/kRunTargetDoesntExist.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_activate_) << "activate_run_target handler was not registered";

    EXPECT_CALL(graph_, setRequestedRunTarget(_)).Times(0);

    ActivateRunTargetResponse response{};
    const ActivateRunTargetRequest request{RunTargetName{"__cp_ut_does_not_exist__"}, ActivationMode::kForced};
    on_activate_(response, request);

    EXPECT_EQ(response.status, RequestStatus::kRejected);
    EXPECT_EQ(response.rejection_reason, ExecErrc::kRunTargetDoesntExist);
}

TEST_F(ControlProviderUT, ActivateRunTarget_SetRequestedRunTargetFails_Rejected)
{
    RecordProperty(
        "Description",
        "When setRequestedRunTarget returns an error, the response carries "
        "kRejected with the same error code propagated from the graph.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_activate_) << "activate_run_target handler was not registered";

    const IdentifierHash known{"cp_ut_known_target_fail"};
    EXPECT_CALL(graph_, setRequestedRunTarget(_)).WillOnce(Return(MakeUnexpected(ExecErrc::kFailed)));

    ActivateRunTargetResponse response{};
    const ActivateRunTargetRequest request{RunTargetName{known.get_name()}, ActivationMode::kForced};
    on_activate_(response, request);

    EXPECT_EQ(response.status, RequestStatus::kRejected);
    EXPECT_EQ(response.rejection_reason, ExecErrc::kFailed);
}

TEST_F(ControlProviderUT, ActivateRunTarget_ForcedSuccess_Accepted)
{
    RecordProperty(
        "Description",
        "A forced activation for a known Run Target that the graph accepts "
        "is acknowledged with kAccepted.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_activate_) << "activate_run_target handler was not registered";

    const IdentifierHash known{"cp_ut_known_target_ok"};
    EXPECT_CALL(graph_, setRequestedRunTarget(_)).WillOnce(Return(score::Result<void>{}));

    ActivateRunTargetResponse response{};
    const ActivateRunTargetRequest request{RunTargetName{known.get_name()}, ActivationMode::kForced};
    on_activate_(response, request);

    EXPECT_EQ(response.status, RequestStatus::kAccepted);
}

// ============================================================================
// handleGetActiveRunTarget
// ============================================================================

TEST_F(ControlProviderUT, GetActiveRunTarget_ActivationInProgress_NotAvailable)
{
    RecordProperty(
        "Description",
        "When the graph reports kActivationInProgress, the response carries "
        "kNotAvailable.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_get_active_) << "get_active_run_target handler was not registered";

    EXPECT_CALL(graph_, getActiveRunTarget()).WillOnce(Return(MakeUnexpected(ExecErrc::kActivationInProgress)));

    GetActiveRunTargetResponse response{};
    on_get_active_(response);

    EXPECT_EQ(response.status, QueryStatus::kNotAvailable);
}

TEST_F(ControlProviderUT, GetActiveRunTarget_Available)
{
    RecordProperty(
        "Description",
        "When the graph returns a valid IdentifierHash, the response carries "
        "kAvailable and the corresponding Run Target name.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_get_active_) << "get_active_run_target handler was not registered";

    const IdentifierHash active{"cp_ut_active_run_target"};
    EXPECT_CALL(graph_, getActiveRunTarget()).WillOnce(Return(score::Result<IdentifierHash>{active}));

    GetActiveRunTargetResponse response{};
    on_get_active_(response);

    EXPECT_EQ(response.status, QueryStatus::kAvailable);
    EXPECT_EQ(response.run_target, RunTargetName{"cp_ut_active_run_target"});
}

TEST_F(ControlProviderUT, GetActiveRunTarget_UnexpectedError_Aborts)
{
    RecordProperty(
        "Description",
        "If getActiveRunTarget() returns any error other than kActivationInProgress "
        "the handler hits an internal assertion and the process terminates, because "
        "no other error code can be communicated to the client.");

    auto provider = MakeProvider();
    ASSERT_TRUE(on_get_active_) << "get_active_run_target handler was not registered";

    EXPECT_CALL(graph_, getActiveRunTarget()).WillRepeatedly(Return(MakeUnexpected(ExecErrc::kFailed)));

    EXPECT_DEATH(
        {
            GetActiveRunTargetResponse response{};
            on_get_active_(response);
        },
        "Impossible to communicate errors other than ExecErrc::kActivationInProgress to the client");
}

// ============================================================================
// handleActivationResult
// ============================================================================

TEST_F(ControlProviderUT, ActivationResult_AllocateFails_SendNotAttempted)
{
    RecordProperty("Description", "When Allocate() fails, the handler returns early without calling Send.");

    auto provider = MakeProvider();

    const IdentifierHash known{"cp_ut_alloc_fail_target"};
    EXPECT_CALL(skeleton_mock_, Allocate()).WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(skeleton_mock_, Send(_)).Times(0);

    TriggerActivation(known, RunTargetActivationSource::kStateManagerRequest);
}

TEST_F(ControlProviderUT, ActivationResult_SendFails_NocrashAfterAllocate)
{
    RecordProperty(
        "Description",
        "When Send() returns an error after a successful Allocate(), the handler "
        "logs and returns cleanly without crashing.");

    auto provider = MakeProvider();

    const IdentifierHash known{"cp_ut_send_fail_target"};
    EXPECT_CALL(skeleton_mock_, Allocate()).WillOnce(Return(score::Result<FakeAllocateePtr>{FakeAllocateePtr{}}));
    EXPECT_CALL(skeleton_mock_, Send(_)).WillOnce(Return(MakeUnexpected(ExecErrc::kCommunicationError)));

    TriggerActivation(known, RunTargetActivationSource::kStateManagerRequest);
}

TEST_F(ControlProviderUT, ActivationResult_Success_EventPopulatedAndSent)
{
    RecordProperty(
        "Description",
        "On the happy path the allocated event is populated with the correct Run "
        "Target name and activation source, then sent via the skeleton.");

    auto provider = MakeProvider();

    const IdentifierHash known{"cp_ut_success_target"};

    FakeAllocateePtr sent{};
    EXPECT_CALL(skeleton_mock_, Allocate()).WillOnce(Return(score::Result<FakeAllocateePtr>{FakeAllocateePtr{}}));
    EXPECT_CALL(skeleton_mock_, Send(_)).WillOnce(DoAll(SaveArg<0>(&sent), Return(score::Result<void>{})));

    TriggerActivation(known, RunTargetActivationSource::kStateManagerRequest);

    EXPECT_EQ(sent.data.activated_run_target, RunTargetName{"cp_ut_success_target"});
    EXPECT_EQ(sent.data.activation_source, RunTargetActivationSource::kStateManagerRequest);
}

// ============================================================================
// offerService — FD_CLOEXEC workaround
// Testing workaround for https://github.com/eclipse-score/communication/issues/1064
//
// Uses the __wrap_fcntl linker wrapper defined at file scope above.
// ScopedFcntlError activates error injection for a specific (fd, cmd) pair
// and restores clean state on destruction.
// ============================================================================

/// @brief RAII guard that activates fcntl() error injection for one (fd, cmd) pair
///        and clears the injection state when it goes out of scope.
struct ScopedFcntlError
{
    ScopedFcntlError(int fd, int cmd, int errno_val = EINVAL)
    {
        g_fcntl_injection = {true, fd, cmd, errno_val};
    }
    ~ScopedFcntlError()
    {
        g_fcntl_injection = {};
    }
};

TEST_F(ControlProviderUT, OfferService_OpenFd_SetsCloexec)
{
    RecordProperty(
        "Description",
        "offerService() sets FD_CLOEXEC on open file descriptors in [0, 16) "
        "except stdin, stdout and stderr, which are explicitly skipped. "
        "A pipe opened before Create() must have the flag set afterwards; "
        "the standard streams must be left completely unchanged.");

    const int stdin_flags_before = fcntl(STDIN_FILENO, F_GETFD);
    const int stdout_flags_before = fcntl(STDOUT_FILENO, F_GETFD);
    const int stderr_flags_before = fcntl(STDERR_FILENO, F_GETFD);
    ASSERT_NE(stdin_flags_before, -1) << "fcntl F_GETFD failed for stdin";
    ASSERT_NE(stdout_flags_before, -1) << "fcntl F_GETFD failed for stdout";
    ASSERT_NE(stderr_flags_before, -1) << "fcntl F_GETFD failed for stderr";

    int fds[2]{-1, -1};
    ASSERT_EQ(pipe(fds), 0) << "pipe() failed";
    ASSERT_LT(fds[0], 16) << "read end of pipe is outside the fd range covered by offerService()";
    ASSERT_LT(fds[1], 16) << "write end of pipe is outside the fd range covered by offerService()";

    auto provider = MakeProvider();

    EXPECT_TRUE(fcntl(fds[0], F_GETFD) & FD_CLOEXEC) << "read end of pipe (fd " << fds[0] << ") did not get FD_CLOEXEC";
    EXPECT_TRUE(fcntl(fds[1], F_GETFD) & FD_CLOEXEC)
        << "write end of pipe (fd " << fds[1] << ") did not get FD_CLOEXEC";

    EXPECT_EQ(fcntl(STDIN_FILENO, F_GETFD), stdin_flags_before) << "offerService() modified stdin flags";
    EXPECT_EQ(fcntl(STDOUT_FILENO, F_GETFD), stdout_flags_before) << "offerService() modified stdout flags";
    EXPECT_EQ(fcntl(STDERR_FILENO, F_GETFD), stderr_flags_before) << "offerService() modified stderr flags";

    close(fds[0]);
    close(fds[1]);
}

TEST_F(ControlProviderUT, OfferService_FSetfdFails_LogsAndContinues)
{
    RecordProperty(
        "Description",
        "When F_SETFD fails for an open file descriptor, offerService() logs a "
        "warning and continues rather than terminating. The failure is injected "
        "via the __wrap_fcntl linker wrapper for the read end of a pipe.");

    int fds[2]{-1, -1};
    ASSERT_EQ(pipe(fds), 0) << "pipe() failed";
    ASSERT_LT(fds[0], 16) << "read end of pipe is outside the fd range covered by offerService()";

    {
        const ScopedFcntlError inject{fds[0], F_SETFD};
        auto provider = MakeProvider();
    }

    close(fds[0]);
    close(fds[1]);
}

// ============================================================================
// LmControlSpecifierFactory — production specifier factory
// ============================================================================

TEST(LmControlSpecifierFactoryUT, Create_ReturnsValidSpecifier)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "LmControlSpecifierFactory::Create() returns a valid mw::com instance "
        "specifier for the production service path without requiring a running runtime.");

    const auto result = LmControlSpecifierFactory::Create();
    EXPECT_TRUE(result.has_value());
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

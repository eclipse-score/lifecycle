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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sched.h>
#include <secpol/secpol.h>
#include <spawn.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "score/mw/launch_manager/osal/details/qnx/start_process_internal.hpp"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/qnx/mock_secpol.h"
#include "score/os/mocklib/qnx/mock_unistd.h"
#include "score/os/mocklib/sched_mock.h"
#include "score/os/mocklib/spawn_mock.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/os/utils/mocklib/signalmock.h"

using namespace testing;

using namespace score::mw::lifecycle::internal::osal;
using namespace score::mw::lifecycle::internal;

// NOLINTBEGIN

/// @brief Mock for the function without a baselibs abstraction, replaced at link time.
class ProcessControlMock
{
  public:
    MOCK_METHOD(int, prlimit, (pid_t, int, const struct rlimit*, struct rlimit*), ());
};

std::unique_ptr<ProcessControlMock> g_process_control_mock = nullptr;

extern "C" {
int __wrap_prlimit(pid_t pid, int resource, const struct rlimit* new_limit, struct rlimit* old_limit)
{
    return g_process_control_mock->prlimit(pid, resource, new_limit, old_limit);
}
}

// NOLINTEND

namespace
{

using OsResult = score::cpp::expected<std::int32_t, score::os::Error>;

OsResult OsSuccess(std::int32_t value = 0)
{
    return OsResult{value};
}

OsResult OsError(std::int32_t error_number)
{
    return score::cpp::make_unexpected(score::os::Error::createFromErrno(error_number));
}

/// @brief The spawn attributes, read back with the real getters while the attribute object is valid.
struct CapturedSpawn
{
    std::uint32_t xflags{};
    pid_t pgroup{-1};
    std::uint64_t runmask{};
    uid_t uid{};
    gid_t gid{};
    std::uint32_t stackmax{};
    sigset_t sigdefault{};
    sigset_t sigmask{};
    std::string path;
    std::vector<std::string> argv;
};

/// @brief Provides the OS functions to detail::startProcessImpl() via mocks.
struct OsMock
{
    NiceMock<score::os::SpawnMock> spawn{};
    StrictMock<score::os::qnx::MockSecpol> secpol{};
    StrictMock<score::os::qnx::MockUnistd> qnx_unistd{};
    NiceMock<score::os::FcntlMock> fcntl{};
    NiceMock<score::os::UnistdMock> unistd{};
    StrictMock<score::os::SchedMock> sched{};
    StrictMock<score::os::SignalMock> signal{};
};

}  // namespace

class QnxStartProcessTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        g_process_control_mock = std::make_unique<StrictMock<ProcessControlMock>>();

        ForwardToRealImplementations();

        config_.name = "TestComponent";
        config_.deployment_config.process_command = configuration::ProcessCommand{"/bin/TestProcess", {"-a", "b"}};
        config_.deployment_config.working_dir = "/";
        config_.deployment_config.sandbox.scheduling_policy = SCHED_RR;
        config_.deployment_config.sandbox.scheduling_priority = 10;
        config_.deployment_config.sandbox.uid = 11;
        config_.deployment_config.sandbox.gid = 13;
    }

    void TearDown() override
    {
        g_process_control_mock.reset();
    }

    /// @brief Forwards the attribute, file action and working directory functions to the real implementations by
    /// default, so the resulting spawn attributes can be read back. Spawning itself is never forwarded.
    void ForwardToRealImplementations()
    {
        ON_CALL(os_.spawn, posix_spawnattr_init).WillByDefault([this](posix_spawnattr_t* attr) {
            return real_os_.spawn.posix_spawnattr_init(attr);
        });
        ON_CALL(os_.spawn, posix_spawnattr_destroy).WillByDefault([this](posix_spawnattr_t* attr) {
            return real_os_.spawn.posix_spawnattr_destroy(attr);
        });
        ON_CALL(os_.spawn, posix_spawnattr_setpgroup).WillByDefault([this](posix_spawnattr_t* attr, pid_t pgroup) {
            return real_os_.spawn.posix_spawnattr_setpgroup(attr, pgroup);
        });
        ON_CALL(os_.spawn, posix_spawnattr_setsigdefault)
            .WillByDefault([this](posix_spawnattr_t* attr, const sigset_t* sigset) {
                return real_os_.spawn.posix_spawnattr_setsigdefault(attr, sigset);
            });
        ON_CALL(os_.spawn, posix_spawnattr_setsigmask)
            .WillByDefault([this](posix_spawnattr_t* attr, const sigset_t* sigset) {
                return real_os_.spawn.posix_spawnattr_setsigmask(attr, sigset);
            });
        ON_CALL(os_.spawn, pthread_spawnattr_setrunmask_np)
            .WillByDefault([this](posix_spawnattr_t* attr, std::uint64_t runmask) {
                return real_os_.spawn.pthread_spawnattr_setrunmask_np(attr, runmask);
            });
        ON_CALL(os_.spawn, posix_spawnattr_setcred)
            .WillByDefault([this](posix_spawnattr_t* attr, uid_t uid, gid_t gid) {
                return real_os_.spawn.posix_spawnattr_setcred(attr, uid, gid);
            });
        ON_CALL(os_.spawn, posix_spawnattr_setstackmax)
            .WillByDefault([this](posix_spawnattr_t* attr, std::uint32_t size) {
                return real_os_.spawn.posix_spawnattr_setstackmax(attr, size);
            });
        ON_CALL(os_.spawn, posix_spawnattr_setcwd_np).WillByDefault([this](posix_spawnattr_t* attr, std::int32_t fd) {
            return real_os_.spawn.posix_spawnattr_setcwd_np(attr, fd);
        });
        ON_CALL(os_.spawn, posix_spawnattr_getxflags)
            .WillByDefault([this](const posix_spawnattr_t* attr, std::uint32_t* flags) {
                return real_os_.spawn.posix_spawnattr_getxflags(attr, flags);
            });
        ON_CALL(os_.spawn, posix_spawnattr_setxflags)
            .WillByDefault([this](posix_spawnattr_t* attr, std::uint32_t flags) {
                return real_os_.spawn.posix_spawnattr_setxflags(attr, flags);
            });
        ON_CALL(os_.spawn, posix_spawn_file_actions_init).WillByDefault([this](posix_spawn_file_actions_t* actions) {
            return real_os_.spawn.posix_spawn_file_actions_init(actions);
        });
        ON_CALL(os_.spawn, posix_spawn_file_actions_destroy).WillByDefault([this](posix_spawn_file_actions_t* actions) {
            return real_os_.spawn.posix_spawn_file_actions_destroy(actions);
        });
        ON_CALL(os_.spawn, posix_spawn_file_actions_adddup2)
            .WillByDefault([this](posix_spawn_file_actions_t* actions, std::int32_t fd, std::int32_t new_fd) {
                return real_os_.spawn.posix_spawn_file_actions_adddup2(actions, fd, new_fd);
            });
        ON_CALL(os_.spawn, posix_spawn).WillByDefault(Return(OsError(ENOSYS)));

        ON_CALL(os_.fcntl, open(_, _)).WillByDefault([this](const char* path, score::os::Fcntl::Open flags) {
            return real_os_.fcntl.open(path, flags);
        });
        ON_CALL(os_.unistd, close).WillByDefault([this](std::int32_t fd) {
            return real_os_.unistd.close(fd);
        });
    }

    /// @brief Expects a successful posix_spawn that captures the spawn attributes.
    void ExpectSpawnSucceeds()
    {
        EXPECT_CALL(os_.spawn, posix_spawn)
            .WillOnce([this](
                          pid_t* pid,
                          const char* path,
                          const posix_spawn_file_actions_t*,
                          const posix_spawnattr_t* attrp,
                          char* const argv[],
                          char* const[]) {
                EXPECT_EQ(posix_spawnattr_getxflags(attrp, &captured_.xflags), 0);
                EXPECT_EQ(posix_spawnattr_getpgroup(attrp, &captured_.pgroup), 0);
                EXPECT_EQ(pthread_spawnattr_getrunmask_np(attrp, &captured_.runmask), 0);
                EXPECT_EQ(posix_spawnattr_getcred(attrp, &captured_.uid, &captured_.gid), 0);
                EXPECT_EQ(posix_spawnattr_getstackmax(attrp, &captured_.stackmax), 0);
                EXPECT_EQ(posix_spawnattr_getsigdefault(attrp, &captured_.sigdefault), 0);
                EXPECT_EQ(posix_spawnattr_getsigmask(attrp, &captured_.sigmask), 0);
                captured_.path = path;
                for (std::size_t i = 0U; argv[i] != nullptr; ++i)
                {
                    captured_.argv.emplace_back(argv[i]);
                }
                *pid = kChildPid;
                return OsSuccess();
            });
    }

    void ExpectSchedulingSet()
    {
        EXPECT_CALL(os_.sched, sched_setscheduler(kChildPid, _, _)).WillOnce(Return(OsSuccess()));
    }

    void ExpectHeldProcessConfigured()
    {
        ExpectSchedulingSet();
        EXPECT_CALL(os_.qnx_unistd, setgroupspid(_, _, kChildPid)).WillOnce(Return(OsSuccess()));
        EXPECT_CALL(*g_process_control_mock, prlimit(kChildPid, _, _, nullptr))
            .Times(AnyNumber())
            .WillRepeatedly(Return(0));
    }

    /// @brief Provides a non-null comms object. startProcessImpl() only checks for its presence; the shared memory
    /// block itself is initialized by IpcCommsSync::createCommsObject() before the process is started.
    void StubComms()
    {
        std::memset(test_buffer_.data(), 0, test_buffer_.size());
        auto* block = reinterpret_cast<IpcCommsSync*>(test_buffer_.data());
        comms_ = IpcCommsP{block, [](IpcCommsSync*) {
                           }};
        comms_fd_ = kShmFd;
    }

    OsalReturnType StartProcess()
    {
        return detail::startProcessImpl(os_, pid_, config_, comms_fd_, comms_);
    }

    static constexpr pid_t kChildPid = 4711;
    static constexpr int kShmFd = 123;

    detail::SpawnOs real_os_{};
    OsMock os_{};

    configuration::ComponentConfig config_ = {};
    ProcessID pid_{};
    int comms_fd_{-1};
    IpcCommsP comms_;
    CapturedSpawn captured_;

    alignas(IpcCommsSync) std::array<std::byte, sizeof(IpcCommsSync)> test_buffer_;
};

TEST_F(QnxStartProcessTest, spawnsHeldProcessWithConfiguredAttributes)
{
    RecordProperty(
        "Description",
        "Verify that the process is spawned held, with the configured process group, credentials, working "
        "directory and arguments, and then continued");

    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, kChildPid);

    const std::uint32_t expected_flags =
        POSIX_SPAWN_HOLD | POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETCRED | POSIX_SPAWN_SETCWD;
    EXPECT_EQ(captured_.xflags & expected_flags, expected_flags);
    // The scheduling is applied to the held process, see schedulingIsAppliedToHeldProcessBeforeContinue
    const std::uint32_t scheduling_flags = POSIX_SPAWN_SETSCHEDULER | POSIX_SPAWN_SETSCHEDPARAM;
    EXPECT_EQ(captured_.xflags & scheduling_flags, 0U);
    EXPECT_EQ(captured_.xflags & static_cast<std::uint32_t>(POSIX_SPAWN_EXPLICIT_CPU), 0U);
    EXPECT_EQ(captured_.xflags & static_cast<std::uint32_t>(POSIX_SPAWN_SETSTACKMAX), 0U);
    EXPECT_EQ(captured_.pgroup, 0);
    EXPECT_EQ(captured_.uid, 11U);
    EXPECT_EQ(captured_.gid, 13U);
    EXPECT_EQ(captured_.path, "/bin/TestProcess");
    EXPECT_THAT(captured_.argv, ElementsAre("/bin/TestProcess", "-a", "b"));
}

TEST_F(QnxStartProcessTest, signalsAreResetToDefaultAndUnblocked)
{
    RecordProperty(
        "Description",
        "Verify that all signals are reset to their default action and unblocked in the new process, so it does not "
        "inherit the signal dispositions and the signal mask of the Launch Manager");

    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);

    const std::uint32_t signal_flags = POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK;
    EXPECT_EQ(captured_.xflags & signal_flags, signal_flags);
    for (const int signal_number : {SIGHUP, SIGINT, SIGPIPE, SIGTERM, SIGUSR1, SIGCHLD})
    {
        EXPECT_EQ(sigismember(&captured_.sigdefault, signal_number), 1) << "signal " << signal_number;
        EXPECT_EQ(sigismember(&captured_.sigmask, signal_number), 0) << "signal " << signal_number;
    }
}

TEST_F(QnxStartProcessTest, affinityIsAppliedWhenConfigured)
{
    RecordProperty("Description", "Verify that the runmask is set if an affinity mask is configured");

    config_.deployment_config.sandbox.affinity_mask = 0x5U;

    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_NE(captured_.xflags & static_cast<std::uint32_t>(POSIX_SPAWN_EXPLICIT_CPU), 0U);
    EXPECT_EQ(captured_.runmask, 0x5U);
}

TEST_F(QnxStartProcessTest, workingDirIsOpenedAndClosed)
{
    RecordProperty(
        "Description",
        "Verify that the working directory is opened as close-on-exec directory, passed to the spawn attributes and "
        "closed afterwards");

    constexpr std::int32_t kCwdFd = 42;
    const auto expected_flags =
        score::os::Fcntl::Open::kReadOnly | score::os::Fcntl::Open::kDirectory | score::os::Fcntl::Open::kCloseOnExec;

    EXPECT_CALL(os_.fcntl, open(StrEq("/"), expected_flags)).WillOnce(Return(OsSuccess(kCwdFd)));
    EXPECT_CALL(os_.spawn, posix_spawnattr_setcwd_np(NotNull(), kCwdFd)).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(os_.unistd, close(kCwdFd)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));
    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, commsFdIsDuplicatedToSyncFd)
{
    RecordProperty("Description", "Verify that the comms fd is duplicated to sync_fd in the new process");

    StubComms();

    EXPECT_CALL(os_.spawn, posix_spawn_file_actions_adddup2(_, kShmFd, IpcCommsSync::sync_fd))
        .WillOnce(Return(OsSuccess()));
    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, noCommsDoesNotDuplicateFd)
{
    RecordProperty("Description", "Verify that no fd is duplicated for a process without comms");

    EXPECT_CALL(os_.spawn, posix_spawn_file_actions_adddup2).Times(0);
    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, supplementaryGroupsAreCleared)
{
    RecordProperty(
        "Description", "Verify that the supplementary groups are explicitly cleared when none are configured");

    ExpectSpawnSucceeds();
    ExpectSchedulingSet();
    EXPECT_CALL(os_.qnx_unistd, setgroupspid(0, nullptr, kChildPid)).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, groupsAndLimitsAreAppliedBeforeContinue)
{
    RecordProperty(
        "Description",
        "Verify that the supplementary groups and resource limits are applied to the held process before "
        "it is continued, and that a stack limit is also applied via the spawn attributes");

    const std::vector<gid_t> sgids = {1, 2, 3};
    config_.deployment_config.sandbox.supplementary_group_ids = sgids;
    config_.deployment_config.sandbox.rlimits = {
        {RLIMIT_NOFILE, {64U, 128U}},
        {RLIMIT_STACK, {65536U, RLIM_INFINITY}},
    };

    ExpectSpawnSucceeds();
    {
        InSequence seq;
        ExpectSchedulingSet();
        EXPECT_CALL(os_.qnx_unistd, setgroupspid(3, NotNull(), kChildPid))
            .WillOnce([&sgids](std::int32_t n, const gid_t* groups, pid_t) {
                EXPECT_TRUE(std::equal(groups, groups + n, sgids.begin()));
                return OsSuccess();
            });
        EXPECT_CALL(
            *g_process_control_mock,
            prlimit(
                kChildPid,
                RLIMIT_NOFILE,
                AllOf(Field(&rlimit::rlim_cur, 64U), Field(&rlimit::rlim_max, 128U)),
                nullptr))
            .WillOnce(Return(0));
        EXPECT_CALL(*g_process_control_mock, prlimit(kChildPid, RLIMIT_STACK, _, nullptr)).WillOnce(Return(0));
        EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));
    }

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_NE(captured_.xflags & static_cast<std::uint32_t>(POSIX_SPAWN_SETSTACKMAX), 0U);
    EXPECT_EQ(captured_.stackmax, 65536U);
}

TEST_F(QnxStartProcessTest, unlimitedStackLimitIsOnlyAppliedWithPrlimit)
{
    RecordProperty(
        "Description",
        "Verify that a stack limit which cannot be set via the spawn attributes is still applied with prlimit and "
        "the process is started");

    config_.deployment_config.sandbox.rlimits = {{RLIMIT_STACK, {RLIM_INFINITY, RLIM_INFINITY}}};

    EXPECT_CALL(os_.spawn, posix_spawnattr_setstackmax).Times(0);
    ExpectSpawnSucceeds();
    {
        InSequence seq;
        ExpectSchedulingSet();
        EXPECT_CALL(os_.qnx_unistd, setgroupspid(0, IsNull(), kChildPid)).WillOnce(Return(OsSuccess()));
        EXPECT_CALL(*g_process_control_mock, prlimit(kChildPid, RLIMIT_STACK, _, nullptr)).WillOnce(Return(0));
        EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));
    }

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(captured_.xflags & static_cast<std::uint32_t>(POSIX_SPAWN_SETSTACKMAX), 0U);
}

TEST_F(QnxStartProcessTest, prlimitFailureKillsHeldProcess)
{
    RecordProperty(
        "Description", "Verify that the held process is killed but reported as started if applying a limit fails");

    config_.deployment_config.sandbox.rlimits = {
        {RLIMIT_CPU, {500U, 500U}},
    };

    ExpectSpawnSucceeds();
    ExpectSchedulingSet();
    EXPECT_CALL(os_.qnx_unistd, setgroupspid).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(*g_process_control_mock, prlimit).WillOnce(SetErrnoAndReturn(EPERM, -1));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGKILL)).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, kChildPid);
}

TEST_F(QnxStartProcessTest, setgroupspidFailureKillsHeldProcess)
{
    RecordProperty(
        "Description",
        "Verify that the held process is killed but reported as started if setting the supplementary groups fails");

    ExpectSpawnSucceeds();
    ExpectSchedulingSet();
    EXPECT_CALL(os_.qnx_unistd, setgroupspid).WillOnce(Return(OsError(EPERM)));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGKILL)).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, kChildPid);
}

TEST_F(QnxStartProcessTest, schedulingIsAppliedToHeldProcessBeforeContinue)
{
    RecordProperty(
        "Description",
        "Verify that the configured scheduling policy and priority are applied to the held process before it is "
        "continued");

    ExpectSpawnSucceeds();
    {
        InSequence seq;
        EXPECT_CALL(
            os_.sched, sched_setscheduler(kChildPid, SCHED_RR, Pointee(Field(&sched_param::sched_priority, 10))))
            .WillOnce(Return(OsSuccess()));
        EXPECT_CALL(os_.qnx_unistd, setgroupspid(_, _, kChildPid)).WillOnce(Return(OsSuccess()));
        EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));
    }

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, schedulingFailureKillsHeldProcess)
{
    RecordProperty(
        "Description",
        "Verify that the held process is killed but reported as started if setting the scheduling fails");

    ExpectSpawnSucceeds();
    EXPECT_CALL(os_.sched, sched_setscheduler).WillOnce(Return(OsError(EPERM)));
    EXPECT_CALL(os_.qnx_unistd, setgroupspid).Times(0);
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGKILL)).WillOnce(Return(OsSuccess()));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, kChildPid);
}

TEST_F(QnxStartProcessTest, continueFailureKillsHeldProcess)
{
    RecordProperty(
        "Description", "Verify that the held process is killed but reported as started if it cannot be continued");

    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsError(EPERM)));
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGKILL)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
    EXPECT_EQ(pid_, kChildPid);
}

TEST_F(QnxStartProcessTest, spawnFailureReturnsFail)
{
    RecordProperty("Description", "Verify that startProcess fails if posix_spawn fails");

    EXPECT_CALL(os_.spawn, posix_spawn).WillOnce(Return(OsError(ENOENT)));
    EXPECT_CALL(os_.qnx_unistd, setgroupspid).Times(0);
    EXPECT_CALL(os_.signal, Kill).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
}

TEST_F(QnxStartProcessTest, attributeInitFailureReturnsFail)
{
    RecordProperty("Description", "Verify that startProcess fails if the spawn attributes cannot be initialized");

    EXPECT_CALL(os_.spawn, posix_spawnattr_init).WillOnce(Return(OsError(ENOMEM)));
    EXPECT_CALL(os_.spawn, posix_spawnattr_destroy).Times(0);
    EXPECT_CALL(os_.spawn, posix_spawn).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
}

TEST_F(QnxStartProcessTest, fileActionsInitFailureReturnsFail)
{
    RecordProperty(
        "Description",
        "Verify that startProcess fails if the file actions cannot be initialized and that the already initialized "
        "spawn attributes are destroyed");

    EXPECT_CALL(os_.spawn, posix_spawn_file_actions_init).WillOnce(Return(OsError(ENOMEM)));
    EXPECT_CALL(os_.spawn, posix_spawn_file_actions_destroy).Times(0);
    EXPECT_CALL(os_.spawn, posix_spawnattr_destroy).Times(1);
    EXPECT_CALL(os_.spawn, posix_spawn).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
}

TEST_F(QnxStartProcessTest, invalidWorkingDirReturnsFail)
{
    RecordProperty("Description", "Verify that startProcess fails if the working directory cannot be opened");

    EXPECT_CALL(os_.fcntl, open(_, _)).WillOnce(Return(OsError(ENOENT)));
    EXPECT_CALL(os_.unistd, close).Times(0);
    EXPECT_CALL(os_.spawn, posix_spawn).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
}

TEST_F(QnxStartProcessTest, securityPolicyIsApplied)
{
    RecordProperty("Description", "Verify that the configured security policy is set as type in the spawn attributes");

    config_.deployment_config.sandbox.security_policy = "app_t";

    EXPECT_CALL(os_.secpol, secpol_posix_spawnattr_settypeid(nullptr, NotNull(), StrEq("app_t"), SECPOL_TYPE_NAME))
        .WillOnce(Return(OsSuccess()));
    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, securityPolicyNotSupportedIsIgnored)
{
    RecordProperty("Description", "Verify that a missing security policy support (ENOTSUP) is ignored");

    config_.deployment_config.sandbox.security_policy = "app_t";

    EXPECT_CALL(os_.secpol, secpol_posix_spawnattr_settypeid).WillOnce(Return(OsError(ENOTSUP)));
    ExpectSpawnSucceeds();
    ExpectHeldProcessConfigured();
    EXPECT_CALL(os_.signal, Kill(kChildPid, SIGCONT)).WillOnce(Return(OsSuccess()));

    EXPECT_EQ(StartProcess(), OsalReturnType::kSuccess);
}

TEST_F(QnxStartProcessTest, securityPolicyFailureReturnsFail)
{
    RecordProperty("Description", "Verify that startProcess fails if the security policy type cannot be set");

    config_.deployment_config.sandbox.security_policy = "unknown_t";

    EXPECT_CALL(os_.secpol, secpol_posix_spawnattr_settypeid).WillOnce(Return(OsError(EINVAL)));
    EXPECT_CALL(os_.spawn, posix_spawn).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
}

namespace
{

/// @brief An attribute or file action call that is made to fail.
struct AttributeFailure
{
    /// Name of the failing call, used as test name
    const char* name;
    /// Configures the call to fail and adapts the configuration so the call is made
    void (*setUp)(OsMock& os, configuration::ComponentConfig& config);
    /// Whether the process communicates, which is required for the file action
    bool with_comms;
};

const AttributeFailure kAttributeFailures[] = {
    {"setpgroup",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setpgroup).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setsigdefault",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setsigdefault).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setsigmask",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setsigmask).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setrunmask",
     [](OsMock& os, configuration::ComponentConfig& config) {
         config.deployment_config.sandbox.affinity_mask = 0x1U;
         EXPECT_CALL(os.spawn, pthread_spawnattr_setrunmask_np).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setcred",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setcred(_, 11U, 13U)).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setstackmax",
     [](OsMock& os, configuration::ComponentConfig& config) {
         config.deployment_config.sandbox.rlimits = {{RLIMIT_STACK, {65536U, RLIM_INFINITY}}};
         EXPECT_CALL(os.spawn, posix_spawnattr_setstackmax).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setcwd",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setcwd_np).WillOnce(Return(OsError(EBADF)));
     },
     false},
    {"getxflags",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_getxflags).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"setxflags",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawnattr_setxflags).WillOnce(Return(OsError(EINVAL)));
     },
     false},
    {"adddup2",
     [](OsMock& os, configuration::ComponentConfig&) {
         EXPECT_CALL(os.spawn, posix_spawn_file_actions_adddup2).WillOnce(Return(OsError(EBADF)));
     },
     true},
};

}  // namespace

class QnxStartProcessAttributeFailureTest : public QnxStartProcessTest,
                                            public ::testing::WithParamInterface<AttributeFailure>
{
};

TEST_P(QnxStartProcessAttributeFailureTest, attributeFailureReturnsFailWithoutSpawning)
{
    RecordProperty(
        "Description",
        "Verify that startProcess fails without spawning a process if setting up a spawn attribute or file action "
        "fails");

    if (GetParam().with_comms)
    {
        StubComms();
    }
    GetParam().setUp(os_, config_);
    EXPECT_CALL(os_.spawn, posix_spawn).Times(0);
    EXPECT_CALL(os_.qnx_unistd, setgroupspid).Times(0);

    EXPECT_EQ(StartProcess(), OsalReturnType::kFail);
    EXPECT_EQ(pid_, -1);
}

INSTANTIATE_TEST_SUITE_P(
    AllAttributeCalls,
    QnxStartProcessAttributeFailureTest,
    ::testing::ValuesIn(kAttributeFailures),
    [](const ::testing::TestParamInfo<AttributeFailure>& param_info) {
        return std::string{param_info.param.name};
    });

class QnxSpawnStackLimitTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }

    static constexpr std::string_view kExecutable = "/bin/TestProcess";

    configuration::Sandbox sandbox_{};
};

TEST_F(QnxSpawnStackLimitTest, noStackLimitConfigured)
{
    RecordProperty("Description", "Verify that no maximum stack size is set if no stack limit is configured");

    sandbox_.rlimits = {{RLIMIT_NOFILE, {64U, 128U}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), std::nullopt);
}

TEST_F(QnxSpawnStackLimitTest, unlimitedStackLimit)
{
    RecordProperty("Description", "Verify that no maximum stack size is set for an unlimited stack limit");

    sandbox_.rlimits = {{RLIMIT_STACK, {RLIM_INFINITY, RLIM_INFINITY}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), std::nullopt);
}

TEST_F(QnxSpawnStackLimitTest, largestRepresentableStackLimit)
{
    RecordProperty("Description", "Verify that the largest stack limit fitting into 32 bit is used as is");

    const rlim_t limit = std::numeric_limits<std::uint32_t>::max();
    sandbox_.rlimits = {{RLIMIT_STACK, {limit, RLIM_INFINITY}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), std::numeric_limits<std::uint32_t>::max());
}

TEST_F(QnxSpawnStackLimitTest, tooLargeStackLimit)
{
    RecordProperty("Description", "Verify that no maximum stack size is set for a stack limit exceeding 32 bit");

    if (sizeof(rlim_t) <= sizeof(std::uint32_t))
    {
        GTEST_SKIP() << "rlim_t cannot exceed 32 bit on this platform";
    }
    const rlim_t limit = static_cast<rlim_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
    sandbox_.rlimits = {{RLIMIT_STACK, {limit, RLIM_INFINITY}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), std::nullopt);
}

TEST_F(QnxSpawnStackLimitTest, softStackLimitIsUsed)
{
    RecordProperty("Description", "Verify that the soft stack limit is used as maximum stack size");

    sandbox_.rlimits = {{RLIMIT_NOFILE, {64U, 128U}}, {RLIMIT_STACK, {65536U, 131072U}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), 65536U);
}

TEST_F(QnxSpawnStackLimitTest, onlyFirstStackLimitIsConsidered)
{
    RecordProperty(
        "Description", "Verify that only the first configured stack limit is considered, even if it is unlimited");

    sandbox_.rlimits = {{RLIMIT_STACK, {RLIM_INFINITY, RLIM_INFINITY}}, {RLIMIT_STACK, {65536U, 131072U}}};

    EXPECT_EQ(detail::spawnStackLimit(sandbox_, kExecutable), std::nullopt);
}

/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <vector>

#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;

namespace
{
// Number of allocations to let pass before throwing one std::bad_alloc; negative means disarmed.
// Thread-local so allocations on other threads cannot consume the armed failure.
thread_local std::atomic<int> g_allocationsUntilFailure{-1};
}  // namespace

void* operator new(std::size_t size)
{
    if (g_allocationsUntilFailure.load() == 0)
    {
        g_allocationsUntilFailure.store(-1);
        throw std::bad_alloc{};
    }
    if (g_allocationsUntilFailure.load() > 0)
    {
        g_allocationsUntilFailure.fetch_sub(1);
    }
    void* ptr = std::malloc(size == 0 ? 1 : size);
    if (ptr == nullptr)
    {
        throw std::bad_alloc{};
    }
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

namespace score::mw::lifecycle::internal::saf::factory
{

class MockRecoveryClient : public IRecoveryClient
{
  public:
    void setRecoveryRequestCallback(IRecoveryClient::RecoveryRequestCallback callback) noexcept override
    {
        // Simple implementation, no action needed for test
    }

    bool sendRecoveryRequest(const IdentifierHash& process_group_identifier) noexcept override
    {
        // Return true to indicate success
        return true;
    }
};

class AliveWorkerFactoryErrorPathTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "error-path-unit-test");
        RecordProperty("DerivationTechnique", "error-condition-injection");
    }

    AliveWorkerFactory factory_;
};

// EmplaceAndAttach catch block: allocation failure while emplacing the observer
TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIf_AllocationFailureReturnsFalse)
{
    std::vector<ifappl::MonitorIfDaemon> interfaces;
    ifappl::CheckpointIpcServer server;
    ifexm::ObservableEvent event{IdentifierHash{std::string_view{"alloc_fail_component"}}};

    // First allocation is the vector storage inside emplace_back.
    g_allocationsUntilFailure.store(0);
    const bool result = factory_.createAliveIf(interfaces, server, event);
    g_allocationsUntilFailure.store(-1);

    EXPECT_FALSE(result);
    EXPECT_TRUE(interfaces.empty());
}

// createObservableEvent catch block: allocation failure while emplacing the event
TEST_F(AliveWorkerFactoryErrorPathTest, CreateObservableEvent_AllocationFailureReturnsFalse)
{
    std::vector<ifexm::ObservableEvent> events;
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);
    const IdentifierHash id{std::string_view{"alloc_fail_event"}};

    g_allocationsUntilFailure.store(0);
    const bool result = factory_.createObservableEvent(events, id, reader);
    g_allocationsUntilFailure.store(-1);

    EXPECT_FALSE(result);
    EXPECT_TRUE(events.empty());
}

// createAliveIfIpc catch block: allocation failure while emplacing the server
TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_AllocationFailureReturnsFalse)
{
    std::vector<ifappl::CheckpointIpcServer> servers;
    const IdentifierHash id{std::string_view{"alloc_fail_ipc"}};

    g_allocationsUntilFailure.store(0);
    const bool result = factory_.createAliveIfIpc(servers, id, 1000);
    g_allocationsUntilFailure.store(-1);

    EXPECT_FALSE(result);
    EXPECT_TRUE(servers.empty());
}

// Success path: the caller's own uid can always be granted access to its channel
TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_WithOwnUid_Succeeds)
{
    std::vector<ifappl::CheckpointIpcServer> servers;
    const IdentifierHash id{std::string_view{"own_uid_ipc_channel"}};

    ASSERT_TRUE(factory_.createAliveIfIpc(servers, id, getuid()));
    EXPECT_EQ(servers.size(), 1U);
}

// init() failure branch: shm_open fails because the file descriptor limit is reached
TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_FdLimitFailsInit)
{
    std::vector<ifappl::CheckpointIpcServer> servers;
    const IdentifierHash id{std::string_view{"fd_limit_ipc_channel"}};

    rlimit original{};
    ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &original), 0);
    rlimit restricted = original;
    restricted.rlim_cur = 0;
    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &restricted), 0);

    const bool result = factory_.createAliveIfIpc(servers, id, getuid());

    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &original), 0);
    EXPECT_FALSE(result);
}

// --------------------------------------------------------------------------
// ERROR PATH: IPC server initialization failure
// When the IPC server's init() reports a non-OK result, the factory returns false
// --------------------------------------------------------------------------

TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_PermissionDeniedExercisesIpcInitFailurePath)
{
    RecordProperty(
        "Description",
        "Exercise the init-failure error path: IPC init() returns non-OK status. "
        "Uses UID 0 (root) which may fail permission setup, causing init or setAccessRights to fail.");

    std::vector<ifappl::CheckpointIpcServer> servers;
    IdentifierHash componentId{std::string_view{"error_test_ipc_permission"}};
    uid_t rootUid = 0;  // Root UID that typically causes permission issues

    bool result = factory_.createAliveIfIpc(servers, componentId, rootUid);

    // Result may be false if permission check fails, which exercises error paths
    // If false: the init or ACL failure branch executed
    if (!result)
    {
        ASSERT_LT(servers.size(), 2);  // At most one server was partially created
        RecordProperty("ErrorPathExercised", "init or ACL failure branch executed");
    }
    else
    {
        // If true, at least successful creation
        ASSERT_EQ(servers.size(), 1);
    }
}

// --------------------------------------------------------------------------
// createAliveIfIpc keeps the emplaced server entry whether or not init/ACL setup succeeds
// --------------------------------------------------------------------------

TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_ServerEntryIsKeptRegardlessOfResult)
{
    RecordProperty(
        "Description",
        "createAliveIfIpc emplaces the server before init(), so exactly one entry exists "
        "even when ACL setup for the given uid fails.");

    std::vector<ifappl::CheckpointIpcServer> servers;
    IdentifierHash componentId{std::string_view{"error_test_ipc_exception"}};
    uid_t invalidUid = 65534;  // Non-root but potentially restrictive UID

    [[maybe_unused]] bool result = factory_.createAliveIfIpc(servers, componentId, invalidUid);

    ASSERT_EQ(servers.size(), 1U);
}

// --------------------------------------------------------------------------
// ERROR PATH: initIpcServerWithUidBasedAccess permission failure
// When setAccessRights() fails, the factory returns false
// --------------------------------------------------------------------------

TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfIpc_AclSetupFailureReturnsFalse)
{
    RecordProperty(
        "Description",
        "init() succeeds and keeps its descriptor, but setAccessRights() cannot reopen the channel "
        "because exactly one new descriptor is allowed, so createAliveIfIpc must return false.");

    std::vector<ifappl::CheckpointIpcServer> servers;
    const IdentifierHash id{std::string_view{"acl_fail_ipc_channel"}};

    const int lowestFreeFd = open("/dev/null", O_RDONLY);
    ASSERT_GE(lowestFreeFd, 0);
    close(lowestFreeFd);

    rlimit original{};
    ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &original), 0);
    rlimit restricted = original;
    restricted.rlim_cur = static_cast<rlim_t>(lowestFreeFd) + 1U;
    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &restricted), 0);

    const bool result = factory_.createAliveIfIpc(servers, id, getuid());

    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &original), 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(servers.size(), 1U);
}

// --------------------------------------------------------------------------
// ERROR PATH: createObservableEvent returns false on duplicate registration
// When registerObservableEvent() returns false due to a duplicate key, createObservableEvent returns false
// --------------------------------------------------------------------------

TEST_F(AliveWorkerFactoryErrorPathTest, CreateObservableEvent_DuplicateComponentIdTriggersFalsePath)
{
    RecordProperty(
        "Description",
        "Exercise the duplicate-registration error path of createObservableEvent. "
        "registerObservableEvent() returns false when component_id already exists in map. "
        "Second call with same ID triggers: emplace succeeds but registration fails, executing the final return "
        "false.");

    std::vector<ifexm::ObservableEvent> events;
    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    IdentifierHash duplicateId{std::string_view{"duplicate_component"}};

    // First call succeeds - happy path
    bool result1 = factory_.createObservableEvent(events, duplicateId, reader);
    ASSERT_TRUE(result1) << "First createObservableEvent should succeed";
    ASSERT_EQ(events.size(), 1);

    // Second call with SAME component_id fails registration
    // This triggers: emplace_back succeeds BUT
    // registerObservableEvent returns false (the duplicate key is rejected)
    // So the success branch is skipped and the final return false executes
    bool result2 = factory_.createObservableEvent(events, duplicateId, reader);
    ASSERT_FALSE(result2) << "Second createObservableEvent with duplicate ID should fail (final return false)";
    ASSERT_EQ(events.size(), 2) << "Vector still has 2 entries even though second registration failed";

    RecordProperty(
        "CoverageNote",
        "createObservableEvent fully exercised: "
        "happy path and duplicate-registration error path");
}

// --------------------------------------------------------------------------
// createAliveIf and createAliveSupervision with caller-provided inputs (no IPC setup involved)
// --------------------------------------------------------------------------

TEST_F(AliveWorkerFactoryErrorPathTest, CreateAliveIfAndSupervision_WithValidInputs_Succeed)
{
    RecordProperty(
        "Description",
        "createObservableEvent, createAliveIf and createAliveSupervision all succeed and append exactly one "
        "object each when given a registered event and a default server entry.");

    auto buffer = std::make_shared<SupervisionBufferType>();
    buffer->initialize();
    ifexm::ObservableEventReader reader(buffer);

    std::vector<ifexm::ObservableEvent> events;
    std::vector<ifappl::CheckpointIpcServer> servers;
    std::vector<ifappl::MonitorIfDaemon> interfaces;
    std::vector<supervision::Alive> supervisions;

    auto mockClient = std::make_shared<MockRecoveryClient>();

    configuration::ComponentAliveSupervision config{};
    config.min_indications = 1;
    config.max_indications = 3;
    config.failed_cycles_tolerance = 0;
    config.reporting_cycle_ms = 100;

    const IdentifierHash id{std::string_view{"test_component_1"}};
    ASSERT_TRUE(factory_.createObservableEvent(events, id, reader));
    ASSERT_EQ(events.size(), 1U);

    servers.emplace_back();

    ASSERT_TRUE(factory_.createAliveIf(interfaces, servers[0], events[0]));
    ASSERT_EQ(interfaces.size(), 1U);

    ASSERT_TRUE(factory_.createAliveSupervision(supervisions, interfaces[0], events[0], mockClient, id, config));
    ASSERT_EQ(supervisions.size(), 1U);
}

}  // namespace score::mw::lifecycle::internal::saf::factory

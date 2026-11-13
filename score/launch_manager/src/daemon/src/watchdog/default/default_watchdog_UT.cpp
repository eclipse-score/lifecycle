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

#include "score/mw/launch_manager/watchdog/default/default_watchdog.hpp"
#include "score/mw/launch_manager/watchdog/watchdog_factory.hpp"

#include "score/memory/shared/shared_memory_factory.h"
#include "score/memory/shared/shared_memory_factory_mock.h"
#include "score/memory/shared/shared_memory_resource_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>

namespace score::mw::lifecycle::internal::watchdog
{
namespace
{

using score::memory::shared::SharedMemoryFactory;
using score::memory::shared::SharedMemoryFactoryMock;
using score::memory::shared::SharedMemoryResourceMock;
using ::testing::_;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::VariantWith;

class DefaultWatchdogTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        SharedMemoryFactory::InjectMock(&factory_mock_);
        ON_CALL(*resource_mock_, do_allocate(sizeof(Heartbeat), alignof(Heartbeat)))
            .WillByDefault(Return(static_cast<void*>(storage_)));
        ON_CALL(*resource_mock_, getUsableBaseAddress()).WillByDefault(Return(static_cast<void*>(storage_)));
    }

    void TearDown() override
    {
        // The injected mock is process-global, so it must not outlive this fixture.
        SharedMemoryFactory::InjectMock(nullptr);
    }

    void expectHeartbeatCreated()
    {
        EXPECT_CALL(factory_mock_, RemoveStaleArtefacts(Eq(path_)));
        EXPECT_CALL(
            factory_mock_,
            Create(Eq(path_), _, sizeof(Heartbeat), VariantWith<SharedMemoryFactory::WorldReadable>(_), false))
            .WillOnce([this](auto&& /*path*/, auto&& callback, auto&&... /*rest*/) {
                callback(resource_mock_);
                return resource_mock_;
            });
    }

    std::uint64_t counter() const
    {
        return std::launder(reinterpret_cast<const Heartbeat*>(storage_))->counter.load();
    }

    const std::string path_{"/lm_heartbeat_ut"};
    StrictMock<SharedMemoryFactoryMock> factory_mock_{};
    std::shared_ptr<NiceMock<SharedMemoryResourceMock>> resource_mock_{
        std::make_shared<NiceMock<SharedMemoryResourceMock>>()};
    /// Stands in for the shared memory region handed out by the resource mock.
    alignas(Heartbeat) std::byte storage_[sizeof(Heartbeat)]{};
    DefaultWatchdog watchdog_{path_};
};

TEST_F(DefaultWatchdogTest, CreateWatchdogReturnsInstance)
{
    RecordProperty("Description", "createWatchdog() returns a watchdog instance.");
    EXPECT_NE(createWatchdog(0, nullptr), nullptr);
}

TEST_F(DefaultWatchdogTest, ServiceAndDeinitWithoutInit_AreNoOps)
{
    RecordProperty(
        "Description", "serviceWatchdog() and deinit() are safe without prior init() and don't touch shared memory.");
    // StrictMock: any call to the factory fails the test.
    watchdog_.serviceWatchdog();
    watchdog_.deinit();
}

TEST_F(DefaultWatchdogTest, Init_RemovesStaleRegionThenCreatesWorldReadableHeartbeat)
{
    RecordProperty(
        "Description",
        "init() first removes a stale region of a previous run, then creates the heartbeat region, only writable by "
        "the owner and readable by everybody, with the counter starting at 0.");
    {
        InSequence sequence;
        expectHeartbeatCreated();
    }

    ASSERT_TRUE(watchdog_.init());
    EXPECT_EQ(counter(), 0U);
}

TEST_F(DefaultWatchdogTest, Init_Fails_WhenRegionCannotBeCreated)
{
    RecordProperty(
        "Description",
        "init() returns false if the region can't be created; serviceWatchdog() and deinit() are no-ops afterwards.");
    EXPECT_CALL(factory_mock_, RemoveStaleArtefacts(Eq(path_)));
    EXPECT_CALL(factory_mock_, Create(Eq(path_), _, _, _, _)).WillOnce(Return(nullptr));

    EXPECT_FALSE(watchdog_.init());
    watchdog_.serviceWatchdog();
    watchdog_.deinit();
}

TEST_F(DefaultWatchdogTest, ServiceWatchdog_IncrementsHeartbeat)
{
    RecordProperty("Description", "Each serviceWatchdog() call increments the heartbeat counter by one.");
    expectHeartbeatCreated();

    ASSERT_TRUE(watchdog_.init());
    watchdog_.serviceWatchdog();
    watchdog_.serviceWatchdog();
    watchdog_.serviceWatchdog();
    EXPECT_EQ(counter(), 3U);
}

TEST_F(DefaultWatchdogTest, Deinit_RemovesHeartbeat)
{
    RecordProperty(
        "Description",
        "deinit() removes the heartbeat region once; serviceWatchdog() and deinit() afterwards are no-ops.");
    expectHeartbeatCreated();
    EXPECT_CALL(factory_mock_, Remove(Eq(path_))).Times(1);

    ASSERT_TRUE(watchdog_.init());
    watchdog_.deinit();
    watchdog_.serviceWatchdog();
    watchdog_.deinit();
    EXPECT_EQ(counter(), 0U);
}

TEST_F(DefaultWatchdogTest, FireWatchdog_ExitsWithFailure)
{
    RecordProperty("Description", "fireWatchdog() terminates the process with exit code EXIT_FAILURE.");
    expectHeartbeatCreated();

    ASSERT_TRUE(watchdog_.init());
    EXPECT_EXIT(watchdog_.fireWatchdog(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::watchdog

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

#include <memory_resource>
#include <new>

#include "score/mw/launch_manager/common/init_only_resource.hpp"

using score::mw::lifecycle::internal::InitOnlyResource;

TEST(InitOnlyResourceTest, AllocationSucceedsBeforeLock)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Allocation succeeds when the resource is not locked.");

    InitOnlyResource resource_;

    void* pointer = resource_.allocate(64, alignof(std::max_align_t));

    ASSERT_NE(pointer, nullptr);
}

TEST(InitOnlyResourceTest, AllocationFailsAfterLock)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Allocation fails when the resource is locked.");

    InitOnlyResource resource_;

    resource_.lock();

    EXPECT_THROW(static_cast<void>(resource_.allocate(64, alignof(std::max_align_t))), std::bad_alloc);
}

TEST(InitOnlyResourceTest, AllocationFailsIfResourceFails)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Allocation fails before lock when the given resource fails.");

    InitOnlyResource resource_{std::pmr::null_memory_resource()};

    EXPECT_THROW(static_cast<void>(resource_.allocate(64, alignof(std::max_align_t))), std::bad_alloc);
}

class MockResource : public std::pmr::memory_resource
{
  public:
    MOCK_METHOD(void*, do_allocate, (std::size_t bytes, std::size_t alignment), (override));
    MOCK_METHOD(void, do_deallocate, (void* p, std::size_t bytes, std::size_t alignment), (override));
    MOCK_METHOD(bool, do_is_equal, (const std::pmr::memory_resource& other), (const, noexcept, override));
};

TEST(InitOnlyResourceTest, AllocationUsesGivenResource)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Allocation before lock is forwarded to the given resource.");

    std::byte buffer[64];
    MockResource mock_resource;
    InitOnlyResource resource_{&mock_resource};

    EXPECT_CALL(mock_resource, do_allocate(64, alignof(std::max_align_t)))
        .WillOnce(testing::Return(static_cast<void*>(buffer)));

    EXPECT_EQ(resource_.allocate(64, alignof(std::max_align_t)), buffer);
}

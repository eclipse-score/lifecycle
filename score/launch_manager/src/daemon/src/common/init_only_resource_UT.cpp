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
#include <gtest/gtest.h>

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

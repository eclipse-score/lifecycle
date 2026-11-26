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

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

TEST(HandleTest, EmptyHandleIsNotAlive)
{
    RecordProperty("Description", "An EmptyHandle does not refer to a resource which can terminate.");

    EXPECT_FALSE(isAlive(EmptyHandle{}));
}

TEST(HandleTest, ProcessHandleIsAlive)
{
    RecordProperty("Description", "A ProcessHandle of a running process is alive.");

    EXPECT_TRUE(isAlive(ProcessHandle{42, nullptr}));
}

TEST(HandleTest, InvalidatedProcessHandleIsNotAlive)
{
    RecordProperty("Description", "A ProcessHandle is no longer alive after it was invalidated.");

    Handle handle = ProcessHandle{42, nullptr};
    invalidate(handle);

    EXPECT_FALSE(isAlive(handle));
}

TEST(HandleTest, InvalidatingEmptyHandleKeepsItEmpty)
{
    RecordProperty("Description", "Invalidating an EmptyHandle leaves it an EmptyHandle.");

    Handle handle = EmptyHandle{};
    invalidate(handle);

    EXPECT_TRUE(std::holds_alternative<EmptyHandle>(handle));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

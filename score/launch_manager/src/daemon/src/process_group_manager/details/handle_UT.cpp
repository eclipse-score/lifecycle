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

TEST(HandleTest, ProcessHandlesWithSamePidAreSameResource)
{
    RecordProperty("Description", "ProcessHandles with the same pid refer to the same resource.");

    EXPECT_TRUE(isSameResource(ProcessHandle{42, nullptr}, ProcessHandle{42, nullptr}));
}

TEST(HandleTest, ProcessHandlesWithDifferentPidAreNotSameResource)
{
    RecordProperty("Description", "ProcessHandles with different pids refer to different resources.");

    EXPECT_FALSE(isSameResource(ProcessHandle{42, nullptr}, ProcessHandle{43, nullptr}));
}

TEST(HandleTest, EmptyHandleIsNeverSameResource)
{
    RecordProperty("Description", "An EmptyHandle does not refer to a resource, so it never matches any handle.");

    EXPECT_FALSE(isSameResource(EmptyHandle{}, EmptyHandle{}));
    EXPECT_FALSE(isSameResource(EmptyHandle{}, ProcessHandle{42, nullptr}));
    EXPECT_FALSE(isSameResource(ProcessHandle{42, nullptr}, EmptyHandle{}));
}

TEST(HandleTest, InvalidatedHandleIsNeverSameResource)
{
    RecordProperty(
        "Description", "An invalidated handle no longer refers to a resource, so it never matches any handle.");

    Handle invalidated = ProcessHandle{42, nullptr};
    invalidate(invalidated);

    EXPECT_FALSE(isSameResource(invalidated, ProcessHandle{42, nullptr}));
    EXPECT_FALSE(isSameResource(ProcessHandle{42, nullptr}, invalidated));
    EXPECT_FALSE(isSameResource(invalidated, invalidated));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

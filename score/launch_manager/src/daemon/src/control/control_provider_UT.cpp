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

#include <type_traits>

#include "score/mw/launch_manager/control/control_provider.hpp"

namespace score::mw::lifecycle::internal
{
namespace
{

// Locks in the exact contract this class was reworked to provide (see the class comment in
// control_provider.hpp): movable via a stable `Impl*` the registered callbacks capture, but
// never copyable, since copying would either alias or duplicate that `Impl`. A regression here
// (e.g. someone reinstating `= delete` on the move ops, or defaulting a copy op) would silently
// reintroduce the dangling-callback bug this Pimpl was introduced to fix, without necessarily
// failing any other test, since nothing else in this suite constructs a `ControlProvider`.
static_assert(std::is_nothrow_move_constructible_v<ControlProvider>);
static_assert(std::is_nothrow_move_assignable_v<ControlProvider>);
static_assert(!std::is_copy_constructible_v<ControlProvider>);
static_assert(!std::is_copy_assignable_v<ControlProvider>);

TEST(ControlProviderUT, StaticAssertionsCompiled)
{
    // The interesting assertions above run at compile time; this keeps the test target from
    // being empty and gives CI something to report.
    SUCCEED();
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

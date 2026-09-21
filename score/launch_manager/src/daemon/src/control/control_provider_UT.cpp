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

#include "score/mw/com/runtime.h"
#include "score/string_manipulation/arguments/arguments.h"

#include <gtest/gtest.h>

#include <optional>
#include <type_traits>
#include <utility>

namespace score::mw::lifecycle::internal
{
namespace
{

// Locks in the exact contract this class was reworked to provide (see the class comment in
// control_provider.hpp): movable via a stable `Impl*` the registered callbacks capture, but
// never copyable, since copying would either alias or duplicate that `Impl`. A regression here
// (e.g. someone reinstating `= delete` on the move ops, or defaulting a copy op) would silently
// reintroduce the dangling-callback bug this Pimpl was introduced to fix.
static_assert(std::is_nothrow_move_constructible_v<ControlProvider>);
static_assert(std::is_nothrow_move_assignable_v<ControlProvider>);
static_assert(!std::is_copy_constructible_v<ControlProvider>);
static_assert(!std::is_copy_assignable_v<ControlProvider>);

// `Create()` never calls back into `graph` before returning, so a no-op stub is sufficient for
// `getActiveRunTarget`/`setRequestedRunTarget`. `registerActiveRunTargetCallback` is captured so
// the test below can invoke it directly, as if a real transition had just completed.
class FakeRunTargetControl : public IRunTargetControl
{
  public:
    [[nodiscard]] score::Result<IdentifierHash> getActiveRunTarget() const noexcept override
    {
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }

    [[nodiscard]] score::Result<void> setRequestedRunTarget(IdentifierHash /*run_target*/) noexcept override
    {
        return {};
    }

    void registerActiveRunTargetCallback(ActivationCallbackT callback) noexcept override
    {
        callback_ = std::move(callback);
    }

    void TriggerActivation(IdentifierHash state, RunTargetActivationSource source)
    {
        ASSERT_TRUE(static_cast<bool>(callback_)) << "registerActiveRunTargetCallback was never called";
        callback_(state, source);
    }

  private:
    ActivationCallbackT callback_;
};

class ControlProviderUT : public ::testing::Test
{
  protected:
    FakeRunTargetControl graph_;
};

TEST_F(ControlProviderUT, StaticAssertionsCompiled)
{
    // The interesting assertions above run at compile time; this keeps the test target from
    // being empty and gives CI something to report.
    SUCCEED();
}

TEST_F(ControlProviderUT, CallbacksDispatchThroughMovedInstance)
{
    RecordProperty(
        "Description",
        "After a ControlProvider is moved, its registered callbacks still dispatch through the "
        "moved-to instance's Impl, not through a stale, already-destroyed one.");

    const IdentifierHash run_target_id{"control_provider_ut_run_target"};

    std::optional<ControlProvider> moved_to;
    {
        Result<ControlProvider> create_result = ControlProvider::Create(&graph_);
        ASSERT_TRUE(create_result.has_value());

        ControlProvider original = std::move(create_result).value();

        // Move `original` into the outer-scoped `moved_to`, then let `original` (and this
        // block) be destroyed. If the registered callback captured `original`'s own address
        // instead of the stable, heap-allocated `Impl*` -- the exact bug the Pimpl in this PR
        // fixes -- that address is gone once this block ends, and triggering the callback below
        // would be a genuine use-after-scope, not just a theoretical one.
        moved_to.emplace(std::move(original));
    }
    ASSERT_TRUE(moved_to.has_value());

    // Doesn't crash and doesn't trip ASan/UBSan iff the callback dispatches through the
    // still-alive `Impl` that `moved_to` now owns.
    graph_.TriggerActivation(run_target_id, RunTargetActivationSource::kStateManagerRequest);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    score::mw::com::runtime::InitializeRuntime(
        score::string_manipulation::GetArguments(argc, const_cast<const char**>(argv)));
    return RUN_ALL_TESTS();
}

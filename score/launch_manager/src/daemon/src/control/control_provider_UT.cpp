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

#include <memory>
#include <type_traits>
#include <utility>

namespace score::mw::lifecycle::internal
{
namespace
{

// Locks in the contract from the class comment in control_provider.hpp: the registered callbacks
// capture the ControlProvider's own address, so it must stay neither movable nor copyable, and
// is handed out as a `std::unique_ptr` instead. A regression here (e.g. someone defaulting the
// move ops) would silently reintroduce a dangling-callback bug.
static_assert(!std::is_move_constructible_v<ControlProvider>);
static_assert(!std::is_move_assignable_v<ControlProvider>);
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

TEST_F(ControlProviderUT, CallbacksDispatchAfterOwnershipTransfer)
{
    RecordProperty(
        "Description",
        "After the std::unique_ptr returned by Create() is moved to a different owner, the "
        "registered callbacks still dispatch through the same, still-alive ControlProvider.");

    const IdentifierHash run_target_id{"control_provider_ut_run_target"};

    std::unique_ptr<ControlProvider> owner;
    {
        Result<std::unique_ptr<ControlProvider>> create_result = ControlProvider::Create(&graph_);
        ASSERT_TRUE(create_result.has_value());

        // Only the pointer moves; the ControlProvider the callbacks captured stays where it is.
        owner = std::move(create_result).value();
    }
    ASSERT_NE(owner, nullptr);

    // Doesn't crash and doesn't trip ASan/UBSan iff the callback dispatches through the
    // still-alive ControlProvider that `owner` now holds.
    graph_.TriggerActivation(run_target_id, RunTargetActivationSource::kStateManagerRequest);
}

TEST_F(ControlProviderUT, DestructionReleasesService)
{
    RecordProperty(
        "Description",
        "Destroying the ControlProvider returned by Create() releases the offered service, so "
        "a later Create() for the same instance succeeds again.");

    Result<std::unique_ptr<ControlProvider>> first = ControlProvider::Create(&graph_);
    ASSERT_TRUE(first.has_value());
    first.value().reset();

    FakeRunTargetControl second_graph;
    Result<std::unique_ptr<ControlProvider>> second = ControlProvider::Create(&second_graph);
    EXPECT_TRUE(second.has_value());
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

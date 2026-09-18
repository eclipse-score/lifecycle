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

namespace score::mw::lifecycle::internal
{
namespace
{

// `Create()` never calls back into `graph` before returning, so a no-op stub is sufficient for
// exercising `Create()` itself: none of these bodies run in the tests below.
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

    void registerActiveRunTargetCallback(ActivationCallbackT /*callback*/) noexcept override {}
};

class ControlProviderUT : public ::testing::Test
{
  protected:
    FakeRunTargetControl graph_;
};

TEST_F(ControlProviderUT, CreateSucceeds)
{
    RecordProperty("Description", "ControlProvider::Create returns a valid instance when the instance is free.");

    const Result<ControlProvider*> result = ControlProvider::Create(&graph_);

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);

    // `ControlProvider` is intentionally never destroyed in production (see the class-level
    // comment: the mw::com callbacks it registers reference it for the daemon's whole lifetime).
    // Free it explicitly here so this test doesn't report that deliberate, process-lifetime
    // "leak" as a `--config=asan_ubsan_lsan` finding of its own, and so the instance below is
    // free to reuse the same instance specifier.
    delete result.value();
}

TEST_F(ControlProviderUT, SecondCreateForSameInstanceFailsCleanly)
{
    RecordProperty("Description",
                    "A second ControlProvider::Create for an already-offered instance fails "
                    "cleanly instead of crashing or hanging.");

    // NOTE: this fails at LmControlSkeleton::Create() itself (an flock on a marker file), before
    // Create() ever reaches the `new ControlProvider{...}` this fix wraps in a unique_ptr guard.
    // It does not exercise that guard's cleanup path. Traced why no config-based trigger exists
    // for the other three setup steps: SkeletonMethod::RegisterHandler (communication's
    // score/mw/com/impl/bindings/lola/skeleton_method.cpp) unconditionally does
    // `type_erased_callback_ = std::move(...); return {};` -- it cannot fail on this binding, so
    // setupActivateRunTarget/setupGetActiveRunTarget can't either, and setupActivationResult's
    // own body is an unconditional `return {};`. That leaves offerService(), whose only reachable
    // failure mode here is genuine OS resource exhaustion (e.g. an artificially lowered FD
    // rlimit) during SHM event-slot allocation -- deliberately not done here, since it'd depend on
    // the binding's internal FD-consumption pattern and risk CI flakiness for little benefit. This
    // path's correctness rests on unique_ptr's RAII guarantee rather than on an executable test.
    const Result<ControlProvider*> first_result = ControlProvider::Create(&graph_);
    ASSERT_TRUE(first_result.has_value());

    const Result<ControlProvider*> second_result = ControlProvider::Create(&graph_);
    EXPECT_FALSE(second_result.has_value());

    delete first_result.value();
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

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

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "score/mw/lifecycle/details/lm_control_impl.hpp"
#include "score/mw/lifecycle/ilm_control.hpp"

namespace score::mw::lifecycle
{
namespace
{

constexpr std::string_view kValidSpecifier = "StateManager/LaunchManager/Instance";

// Tests inject FakeLmControl via CreateLmControl<Impl> (#688); the mw::com-backed
// implementation is covered by lm_control_impl_UT.cpp.

/// @brief Fake ILmControl with a scriptable init() result; state is static
/// because CreateLmControl default-constructs its Impl. reset() between tests.
class FakeLmControl final : public ILmControl
{
  public:
    score::Result<void> init(std::string_view instance_specifier) noexcept
    {
        ++init_call_count;
        last_instance_specifier = std::string{instance_specifier};
        if (next_init_error.has_value())
        {
            return score::MakeUnexpected(next_init_error.value());
        }
        return {};
    }

    score::Result<void> wait_for_connection(std::chrono::milliseconds, const score::cpp::stop_token&) override
    {
        return {};
    }

    score::Result<void> activate_run_target(RunTargetName, bool) override
    {
        return {};
    }

    score::Result<void> register_run_target_activation_callback(ActivationCallback) override
    {
        return {};
    }

    score::Result<RunTargetName> get_active_run_target() override
    {
        return RunTargetName{kValidSpecifier};
    }

    static void reset() noexcept
    {
        next_init_error.reset();
        init_call_count = 0U;
        last_instance_specifier.clear();
    }

    static std::optional<ExecErrc> next_init_error;
    static std::size_t init_call_count;
    static std::string last_instance_specifier;
};

std::optional<ExecErrc> FakeLmControl::next_init_error{};
std::size_t FakeLmControl::init_call_count{0U};
std::string FakeLmControl::last_instance_specifier{};

class ILmControlUT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        // The following two equivalence classes are tested here:
        // * Return a valid instance
        // * Return an error
        // The different error cases are tested in the underlying impl class LmControlImpl unit tests

        FakeLmControl::reset();
    }
};

TEST_F(ILmControlUT, ReturnsValidInstanceAndForwardsSpecifier)
{
    RecordProperty(
        "Description",
        "CreateLmControl returns a valid instance when init() succeeds, "
        "initializing the implementation exactly once with the given instance specifier.");

    auto result = internal::CreateLmControl<FakeLmControl>(kValidSpecifier);

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
    EXPECT_EQ(FakeLmControl::init_call_count, 1U);
    EXPECT_EQ(FakeLmControl::last_instance_specifier, kValidSpecifier);
}

TEST_F(ILmControlUT, PropagatesInitError)
{
    RecordProperty("Description", "CreateLmControl forwards the error that init() reports.");

    FakeLmControl::next_init_error = ExecErrc::kCommunicationError;

    auto result = internal::CreateLmControl<FakeLmControl>(kValidSpecifier);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(ILmControlUT, ReturnsInitErrorForInvalidSpecifier)
{
    RecordProperty(
        "Description",
        "ILmControl::Create returns kInvalidArguments error for an invalid instance specifier. The specifier "
        "is rejected by validation before any communication is attempted, so no mw::com runtime is required.");

    auto result = ILmControl::Create("");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kInvalidArguments);
}

}  // namespace
}  // namespace score::mw::lifecycle

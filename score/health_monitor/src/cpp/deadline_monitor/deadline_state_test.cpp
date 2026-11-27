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

#include "score/health_monitor/src/cpp/deadline_monitor/deadline_state.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>

using namespace score::health_monitor::details;

namespace
{

constexpr std::uint32_t kLargestTimestamp{std::numeric_limits<std::uint32_t>::max()};

}  // namespace

TEST(DeadlineStateSnapshotTest, DefaultSnapshotIsStoppedAndNotRunningNorTooEarly)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a default snapshot is stopped, not running, not too early, at time 0.");

    const DeadlineStateSnapshot snapshot{};

    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_FALSE(snapshot.IsRunning());
    EXPECT_FALSE(snapshot.IsTooEarly());
    EXPECT_EQ(snapshot.TimestampMs(), 0U);
}

TEST(DeadlineStateSnapshotTest, SettingTheTimestampKeepsTheFlags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that replacing the timestamp leaves the stopped, running and early flags.");

    DeadlineStateSnapshot snapshot{};
    snapshot.SetRunning();
    snapshot.SetTooEarly();

    snapshot.SetTimestampMs(1234U);

    EXPECT_EQ(snapshot.TimestampMs(), 1234U);
    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_TRUE(snapshot.IsRunning());
    EXPECT_TRUE(snapshot.IsTooEarly());
}

TEST(DeadlineStateSnapshotTest, SettingTheRunningFlagKeepsTheOtherFields)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the running flag does not change the timestamp or the early flag.");

    DeadlineStateSnapshot snapshot{};
    snapshot.SetTimestampMs(77U);

    snapshot.SetRunning();

    EXPECT_TRUE(snapshot.IsRunning());
    EXPECT_FALSE(snapshot.IsTooEarly());
    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_EQ(snapshot.TimestampMs(), 77U);
}

TEST(DeadlineStateSnapshotTest, SettingTheTooEarlyFlagKeepsTheOtherFields)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the early flag does not change the timestamp or the running flag.");

    DeadlineStateSnapshot snapshot{};
    snapshot.SetTimestampMs(88U);

    snapshot.SetTooEarly();

    EXPECT_TRUE(snapshot.IsTooEarly());
    EXPECT_FALSE(snapshot.IsRunning());
    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_EQ(snapshot.TimestampMs(), 88U);
}

TEST(DeadlineStateSnapshotTest, RawValueRoundTrips)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a snapshot built from a packed value returns it and reads its fields.");

    const std::uint64_t raw{(std::uint64_t{42U} << 32U) | 0b1010U};

    const DeadlineStateSnapshot snapshot{raw};

    EXPECT_EQ(snapshot.Raw(), raw);
    EXPECT_EQ(snapshot.TimestampMs(), 42U);
    EXPECT_TRUE(snapshot.IsRunning());
    EXPECT_TRUE(snapshot.IsTooEarly());
    EXPECT_FALSE(snapshot.IsStopped());
}

TEST(DeadlineStateSnapshotTest, LargestTimestampRoundTrips)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the timestamp 2^32 - 1 is stored without loss and without flag changes.");

    DeadlineStateSnapshot snapshot{};
    snapshot.SetRunning();

    snapshot.SetTimestampMs(kLargestTimestamp);

    EXPECT_EQ(snapshot.TimestampMs(), kLargestTimestamp);
    EXPECT_TRUE(snapshot.IsRunning());
    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_FALSE(snapshot.IsTooEarly());
}

TEST(DeadlineStateTest, NewStateIsStopped)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a new state reads as the default snapshot.");

    const DeadlineState state{};

    const auto snapshot = state.Snapshot();
    EXPECT_TRUE(snapshot.IsStopped());
    EXPECT_FALSE(snapshot.IsRunning());
    EXPECT_FALSE(snapshot.IsTooEarly());
    EXPECT_EQ(snapshot.TimestampMs(), 0U);
}

TEST(DeadlineStateTest, UpdateStoresTheResultAndReturnsThePreviousSnapshot)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that Update sets running and the timestamp and returns the earlier state.");

    DeadlineState state{};

    const auto result = state.Update([](DeadlineStateSnapshot snapshot) -> std::optional<DeadlineStateSnapshot> {
        snapshot.SetTimestampMs(500U);
        snapshot.SetRunning();
        return snapshot;
    });

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().IsRunning());
    EXPECT_TRUE(result.value().IsStopped());
    EXPECT_EQ(result.value().TimestampMs(), 0U);
    const auto snapshot = state.Snapshot();
    EXPECT_TRUE(snapshot.IsRunning());
    EXPECT_EQ(snapshot.TimestampMs(), 500U);
}

TEST(DeadlineStateTest, UpdateReturningNothingKeepsTheStateAndReturnsAnError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that an Update without a result leaves the state and returns it as error.");

    DeadlineState state{};
    ASSERT_TRUE(state
                    .Update([](DeadlineStateSnapshot snapshot) -> std::optional<DeadlineStateSnapshot> {
                        snapshot.SetTimestampMs(9U);
                        snapshot.SetRunning();
                        return snapshot;
                    })
                    .has_value());
    const auto before = state.Snapshot().Raw();

    const auto result = state.Update([](DeadlineStateSnapshot) -> std::optional<DeadlineStateSnapshot> {
        return std::nullopt;
    });

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().Raw(), before);
    EXPECT_EQ(state.Snapshot().Raw(), before);
}

TEST(DeadlineStateTest, UpdateReceivesTheCurrentSnapshot)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a second Update sees the result of the first one.");

    DeadlineState state{};
    ASSERT_TRUE(state
                    .Update([](DeadlineStateSnapshot snapshot) -> std::optional<DeadlineStateSnapshot> {
                        snapshot.SetTimestampMs(31U);
                        return snapshot;
                    })
                    .has_value());

    std::uint64_t seen{0U};
    const auto result = state.Update([&seen](DeadlineStateSnapshot snapshot) -> std::optional<DeadlineStateSnapshot> {
        seen = snapshot.TimestampMs();
        snapshot.SetTooEarly();
        return snapshot;
    });

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(seen, 31U);
    EXPECT_TRUE(state.Snapshot().IsTooEarly());
    EXPECT_EQ(state.Snapshot().TimestampMs(), 31U);
}

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

#include "score/health_monitor/src/cpp/heartbeat_monitor/heartbeat_state.h"

#include <gtest/gtest.h>
#include <score/utility.hpp>

#include <atomic>
#include <cstdint>
#include <optional>
#include <thread>

using namespace score::health_monitor::details;

namespace
{

void RecordHeartbeat(HeartbeatState& state, const std::uint64_t timestamp)
{
    score::cpp::ignore = state.Update([timestamp](HeartbeatStateSnapshot current) {
        current.SetHeartbeatTimestamp(timestamp);
        current.IncrementCounter();
        return std::optional<HeartbeatStateSnapshot>{current};
    });
}

}  // namespace

TEST(HeartbeatStateSnapshotTest, NewSnapshotHasNoHeartbeat)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a new snapshot holds no timestamp and no heartbeat.");

    const HeartbeatStateSnapshot snapshot{};

    EXPECT_EQ(snapshot.Raw(), 0U);
    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 0U);
    EXPECT_EQ(snapshot.Counter(), 0U);
}

TEST(HeartbeatStateSnapshotTest, PackedValueHoldsTimestampAboveCounter)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the packed value holds the timestamp above the counter bits.");

    const std::uint64_t raw{(std::uint64_t{1234U} << 2U) | 2U};

    const HeartbeatStateSnapshot snapshot{raw};

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 1234U);
    EXPECT_EQ(snapshot.Counter(), 2U);
    EXPECT_EQ(snapshot.Raw(), raw);
}

TEST(HeartbeatStateSnapshotTest, SettingTimestampKeepsCounter)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that setting the timestamp keeps the counter.");

    HeartbeatStateSnapshot snapshot{};
    snapshot.IncrementCounter();
    snapshot.IncrementCounter();

    snapshot.SetHeartbeatTimestamp(500U);

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 500U);
    EXPECT_EQ(snapshot.Counter(), 2U);
}

TEST(HeartbeatStateSnapshotTest, IncrementingCounterKeepsTimestamp)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that incrementing the counter keeps the timestamp.");

    HeartbeatStateSnapshot snapshot{};
    snapshot.SetHeartbeatTimestamp(500U);

    snapshot.IncrementCounter();

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 500U);
    EXPECT_EQ(snapshot.Counter(), 1U);
}

TEST(HeartbeatStateSnapshotTest, CounterStopsAtMaximum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the counter stays at its maximum when incremented beyond it.");

    HeartbeatStateSnapshot snapshot{};

    for (int i{0}; i < kHeartbeatCounterMax + 2; ++i)
    {
        snapshot.IncrementCounter();
    }

    EXPECT_EQ(snapshot.Counter(), kHeartbeatCounterMax);
}

TEST(HeartbeatStateSnapshotTest, LargestTimestampKeepsCounter)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the largest timestamp fits without changing the counter.");

    HeartbeatStateSnapshot snapshot{};
    snapshot.IncrementCounter();

    snapshot.SetHeartbeatTimestamp(kHeartbeatTimestampLimit - 1U);

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), kHeartbeatTimestampLimit - 1U);
    EXPECT_EQ(snapshot.Counter(), 1U);
}

TEST(HeartbeatStateSnapshotDeathTest, TimestampAtLimitAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a timestamp at the packing limit aborts.");

    HeartbeatStateSnapshot snapshot{};

    EXPECT_DEATH(snapshot.SetHeartbeatTimestamp(kHeartbeatTimestampLimit), "");
}

TEST(HeartbeatStateTest, NewStateHasNoHeartbeat)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a new state holds no heartbeat.");

    HeartbeatState state{};

    EXPECT_EQ(state.Reset().Raw(), 0U);
}

TEST(HeartbeatStateTest, UpdateStoresNewSnapshotAndReturnsPrevious)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that an update stores the new state and returns the state before it.");

    HeartbeatState state{};
    RecordHeartbeat(state, 10U);
    const HeartbeatStateSnapshot replacement{std::uint64_t{0xABCDU}};

    const auto result = state.Update([replacement](const HeartbeatStateSnapshot) {
        return std::optional<HeartbeatStateSnapshot>{replacement};
    });

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().HeartbeatTimestamp(), 10U);
    EXPECT_EQ(result.value().Counter(), 1U);
    EXPECT_EQ(state.Reset().Raw(), replacement.Raw());
}

TEST(HeartbeatStateTest, DeclinedUpdateKeepsState)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that an update without a new state keeps the state and returns it as error.");

    HeartbeatState state{};
    RecordHeartbeat(state, 10U);

    const auto result = state.Update([](const HeartbeatStateSnapshot) {
        return std::optional<HeartbeatStateSnapshot>{};
    });

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().HeartbeatTimestamp(), 10U);
    EXPECT_EQ(result.error().Counter(), 1U);
    const HeartbeatStateSnapshot snapshot{state.Reset()};
    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 10U);
    EXPECT_EQ(snapshot.Counter(), 1U);
}

TEST(HeartbeatStateTest, BeatsKeepLatestTimestampAndCountThem)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that two heartbeats keep the latest timestamp and a count of two.");

    HeartbeatState state{};
    RecordHeartbeat(state, 10U);
    RecordHeartbeat(state, 20U);

    const HeartbeatStateSnapshot snapshot{state.Reset()};

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 20U);
    EXPECT_EQ(snapshot.Counter(), 2U);
}

TEST(HeartbeatStateTest, CountStopsAtMaximumOverManyBeats)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that ten heartbeats keep the latest timestamp and the maximum count.");

    HeartbeatState state{};
    for (std::uint64_t timestamp{1U}; timestamp <= 10U; ++timestamp)
    {
        RecordHeartbeat(state, timestamp);
    }

    const HeartbeatStateSnapshot snapshot{state.Reset()};

    EXPECT_EQ(snapshot.HeartbeatTimestamp(), 10U);
    EXPECT_EQ(snapshot.Counter(), kHeartbeatCounterMax);
}

TEST(HeartbeatStateTest, ResetStartsCycleWithoutHeartbeat)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_too_late");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a reset leaves the state without a heartbeat.");

    HeartbeatState state{};
    RecordHeartbeat(state, 1234U);
    score::cpp::ignore = state.Reset();

    EXPECT_EQ(state.Reset().Raw(), 0U);
}

TEST(HeartbeatStateDeathTest, HeartbeatAtTimestampLimitAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a heartbeat at the timestamp limit aborts.");

    HeartbeatState state{};

    EXPECT_DEATH(RecordHeartbeat(state, kHeartbeatTimestampLimit), "");
}

TEST(HeartbeatStateTest, HeartbeatRacingWithResetLandsInExactlyOneCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency, comp_req__health_monitor__hb_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a heartbeat racing a reset counts in exactly one cycle.");

    const int rounds{1000};
    for (int round{0}; round < rounds; ++round)
    {
        HeartbeatState state{};
        std::atomic<bool> go{false};
        std::thread beater{[&state, &go] {
            while (!go.load())
            {
                std::this_thread::yield();
            }

            RecordHeartbeat(state, 1U);
        }};

        go.store(true);
        const HeartbeatStateSnapshot racing{state.Reset()};
        beater.join();
        const HeartbeatStateSnapshot after{state.Reset()};

        ASSERT_EQ(racing.Counter() + after.Counter(), 1U) << "round " << round;
    }
}

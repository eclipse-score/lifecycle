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

#include "score/health_monitor/src/cpp/logic_monitor/logic_state.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>

using namespace score::health_monitor::details;

TEST(LogicStateSnapshotTest, NewSnapshotHoldsIndexAndIsHealthy)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a new snapshot holds the index above a healthy status byte.");

    const LogicStateSnapshot snapshot{5U};

    EXPECT_EQ(snapshot.CurrentStateIndex(), 5U);
    EXPECT_EQ(snapshot.Raw(), std::uint64_t{5U} << 8U);
    EXPECT_FALSE(snapshot.MonitorStatus().has_value());
}

TEST(LogicStateSnapshotTest, SnapshotFromZeroIsHealthyAtIndexZero)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the packed value 0 reads as state index 0 and healthy.");

    const auto snapshot = LogicStateSnapshot::FromRaw(0U);

    EXPECT_EQ(snapshot.CurrentStateIndex(), 0U);
    EXPECT_FALSE(snapshot.MonitorStatus().has_value());
    EXPECT_EQ(snapshot.Raw(), 0U);
}

TEST(LogicStateSnapshotTest, SnapshotFromValidPackedValueReadsIndexAndStatus)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a packed value with the status byte 1 reads as kInvalidState.");

    const std::uint64_t raw{0xDEADBEEFDEADBE01U};
    const auto snapshot = LogicStateSnapshot::FromRaw(raw);

    EXPECT_EQ(snapshot.CurrentStateIndex(), static_cast<std::size_t>(raw >> 8U));
    EXPECT_EQ(snapshot.MonitorStatus(), std::optional<LogicEvaluationError>{LogicEvaluationError::kInvalidState});
    EXPECT_EQ(snapshot.Raw(), raw);
}

TEST(LogicStateSnapshotTest, SnapshotFromMaximumPackedValueReadsUnmappedError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that a status byte that is no enumerator reads as kUnmappedError.");

    const auto snapshot = LogicStateSnapshot::FromRaw(UINT64_MAX);

    EXPECT_EQ(snapshot.MonitorStatus(), std::optional<LogicEvaluationError>{LogicEvaluationError::kUnmappedError});
    EXPECT_EQ(snapshot.CurrentStateIndex(), static_cast<std::size_t>(UINT64_MAX >> 8U));
}

TEST(LogicStateSnapshotTest, SettingTheIndexKeepsTheStatus)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that replacing the state index does not change the status.");

    auto snapshot = LogicStateSnapshot::FromRaw(0xDEADBEEFDEADBE02U);

    snapshot.SetCurrentStateIndex(7U);

    EXPECT_EQ(snapshot.CurrentStateIndex(), 7U);
    EXPECT_EQ(snapshot.MonitorStatus(), std::optional<LogicEvaluationError>{LogicEvaluationError::kInvalidTransition});
}

TEST(LogicStateSnapshotTest, SettingTheStatusKeepsTheIndex)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that replacing the status does not change the state index.");

    LogicStateSnapshot snapshot{9U};

    snapshot.SetMonitorStatus(LogicEvaluationError::kInvalidTransition);

    EXPECT_EQ(snapshot.CurrentStateIndex(), 9U);
    EXPECT_EQ(snapshot.MonitorStatus(), std::optional<LogicEvaluationError>{LogicEvaluationError::kInvalidTransition});
}

TEST(LogicStateSnapshotTest, SettingTheIndexAtTheLimitDies)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a state index of 2^56 violates the precondition and terminates.");

    LogicStateSnapshot snapshot{0U};

    EXPECT_DEATH(snapshot.SetCurrentStateIndex(static_cast<std::size_t>(1U) << 56U), "");
}

TEST(LogicStateTest, NewStateStartsHealthyAtTheInitialIndex)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a new state holds the initial index and no failure.");

    const LogicState state{3U};

    EXPECT_EQ(state.Snapshot().CurrentStateIndex(), 3U);
    EXPECT_FALSE(state.Snapshot().MonitorStatus().has_value());
}

TEST(LogicStateTest, SwapReturnsThePreviousSnapshotAndStoresTheNewOne)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial, comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that Swap returns the earlier snapshot and a later Snapshot shows the new one.");

    LogicState state{1U};
    LogicStateSnapshot next{2U};
    next.SetMonitorStatus(LogicEvaluationError::kInvalidTransition);

    const auto previous = state.Swap(next);

    EXPECT_EQ(previous.CurrentStateIndex(), 1U);
    EXPECT_FALSE(previous.MonitorStatus().has_value());
    EXPECT_EQ(state.Snapshot().Raw(), next.Raw());
}

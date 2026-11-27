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

#include "score/health_monitor/src/cpp/internal/fetch_update.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <optional>

using namespace score::health_monitor::details;

namespace
{

const std::uint64_t kInitial{5U};
const std::uint64_t kStoredMeanwhile{10U};

}  // namespace

TEST(FetchUpdateTest, StoresNewValueAndReturnsPrevious)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that an update stores the new value and returns the value before it.");

    std::atomic<std::uint64_t> atomic{kInitial};

    const auto result =
        FetchUpdate(atomic, std::memory_order_acq_rel, std::memory_order_acquire, [](const std::uint64_t value) {
            return std::optional<std::uint64_t>{value + 1U};
        });

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kInitial);
    EXPECT_EQ(atomic.load(), kInitial + 1U);
}

TEST(FetchUpdateTest, DeclinedUpdateLeavesValueAndReturnsItAsError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that an update without a new value keeps the value and returns it as error.");

    std::atomic<std::uint64_t> atomic{kInitial};

    const auto result =
        FetchUpdate(atomic, std::memory_order_acq_rel, std::memory_order_acquire, [](const std::uint64_t) {
            return std::optional<std::uint64_t>{};
        });

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), kInitial);
    EXPECT_EQ(atomic.load(), kInitial);
}

TEST(FetchUpdateTest, RetriesWithValueStoredMeanwhile)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that an update repeats with the value another writer stored in between.");

    std::atomic<std::uint64_t> atomic{kInitial};
    bool first_call{true};
    std::uint64_t last_seen{0U};

    const auto result = FetchUpdate(
        atomic,
        std::memory_order_acq_rel,
        std::memory_order_acquire,
        [&atomic, &first_call, &last_seen](const std::uint64_t value) {
            if (first_call)
            {
                first_call = false;
                atomic.store(kStoredMeanwhile);
            }
            last_seen = value;
            return std::optional<std::uint64_t>{value + 1U};
        });

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kStoredMeanwhile);
    EXPECT_EQ(last_seen, kStoredMeanwhile);
    EXPECT_EQ(atomic.load(), kStoredMeanwhile + 1U);
}

TEST(FetchUpdateTest, DeclinedRetryReturnsLatestValueAsError)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that an update declined on retry returns the value another writer stored as error.");

    std::atomic<std::uint64_t> atomic{kInitial};

    const auto result =
        FetchUpdate(atomic, std::memory_order_acq_rel, std::memory_order_acquire, [&atomic](const std::uint64_t value) {
            if (value == kInitial)
            {
                atomic.store(kStoredMeanwhile);
                return std::optional<std::uint64_t>{value + 1U};
            }
            return std::optional<std::uint64_t>{};
        });

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), kStoredMeanwhile);
    EXPECT_EQ(atomic.load(), kStoredMeanwhile);
}

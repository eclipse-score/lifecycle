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

#include "score/health_monitor/taken.h"

#include "score/health_monitor/mock_taken.h"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

using namespace score::health_monitor;
using score::health_monitor::testing::MakeTaken;

namespace
{

struct Counter
{
    int value{0};
};

struct DerivedCounter : Counter
{
};

struct DestructionFlag
{
    explicit DestructionFlag(bool& destroyed) noexcept : destroyed_{destroyed}
    {
    }
    DestructionFlag(const DestructionFlag&) = delete;
    DestructionFlag(DestructionFlag&&) = delete;
    DestructionFlag& operator=(const DestructionFlag&) = delete;
    DestructionFlag& operator=(DestructionFlag&&) = delete;
    ~DestructionFlag()
    {
        destroyed_ = true;
    }

    bool& destroyed_;
};

}  // namespace

static_assert(
    std::is_convertible<Taken<DerivedCounter>&&, Taken<Counter>>::value,
    "A handle to a derived type must convert to a handle to its base");
static_assert(
    !std::is_convertible<Taken<Counter>&&, Taken<DerivedCounter>>::value,
    "A handle to a base must not convert to a handle to a derived type");

static_assert(!std::is_copy_constructible<Taken<Counter>>::value, "Taken must not be copyable");
static_assert(!std::is_copy_assignable<Taken<Counter>>::value, "Taken must not be copy-assignable");
static_assert(std::is_nothrow_move_constructible<Taken<Counter>>::value, "Taken must move without throwing");
static_assert(std::is_nothrow_move_assignable<Taken<Counter>>::value, "Taken must move-assign without throwing");
static_assert(!std::is_default_constructible<Taken<Counter>>::value, "Taken must not exist without an object");

TEST(TakenTest, AccessorsReturnTheTakenObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that every accessor of a taken handle returns the taken object.");

    Counter counter{};
    const auto taken{MakeTaken(counter)};

    EXPECT_EQ(&taken.Get(), &counter);
    EXPECT_EQ(&*taken, &counter);
    EXPECT_EQ(taken.operator->(), &counter);
}

TEST(TakenTest, ArrowWritesToTheTakenObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a write through the arrow operator reaches the taken object.");

    Counter counter{};
    const auto taken{MakeTaken(counter)};

    taken->value = 7;

    EXPECT_EQ(counter.value, 7);
}

TEST(TakenTest, MoveConstructionTransfersTheObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that move construction hands the taken object to the new handle.");

    Counter counter{};
    auto source{MakeTaken(counter)};

    const Taken<Counter> target{std::move(source)};

    EXPECT_EQ(&target.Get(), &counter);
}

TEST(TakenTest, MoveAssignmentTransfersTheObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that move assignment hands the taken object to the target handle.");

    Counter first{};
    Counter second{};
    auto source{MakeTaken(first)};
    auto target{MakeTaken(second)};

    target = std::move(source);

    EXPECT_EQ(&target.Get(), &first);
}

TEST(TakenTest, ConversionToBaseTransfersTheObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that converting to a handle of the base type keeps the taken object.");

    DerivedCounter counter{};
    auto source{MakeTaken(counter)};

    const Taken<Counter> target{std::move(source)};

    EXPECT_EQ(&target.Get(), &counter);
}

TEST(TakenTest, KeepsTheObjectAliveAfterTheOtherOwnerReleasesIt)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a taken object outlives the release of its other owner.");

    bool destroyed{false};
    auto owner{std::make_shared<DestructionFlag>(destroyed)};
    const Taken<DestructionFlag> taken{owner};

    owner.reset();

    EXPECT_FALSE(destroyed);
    EXPECT_EQ(&taken->destroyed_, &destroyed);
}

TEST(TakenTest, DestroysTheObjectWithTheLastOwner)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that the taken object is destroyed when its last owner is gone.");

    bool destroyed{false};
    auto owner{std::make_shared<DestructionFlag>(destroyed)};
    {
        const Taken<DestructionFlag> taken{owner};
        owner.reset();
    }

    EXPECT_TRUE(destroyed);
}

TEST(TakenTest, MoveKeepsTheUseCount)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that moving a handle does not add an owner of the taken object.");

    auto owner{std::make_shared<Counter>()};
    const std::weak_ptr<Counter> observer{owner};
    auto source{Taken<Counter>{owner}};
    ASSERT_EQ(observer.use_count(), 2);

    const Taken<Counter> target{std::move(source)};

    EXPECT_EQ(observer.use_count(), 2);
}

TEST(TakenTest, ConversionToBaseKeepsTheSharedOwnership)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that a handle converted to the base type stays the only owner.");

    auto owner{std::make_shared<DerivedCounter>()};
    const std::weak_ptr<DerivedCounter> observer{owner};
    Taken<DerivedCounter> source{owner};
    owner.reset();

    const Taken<Counter> target{std::move(source)};

    EXPECT_EQ(observer.use_count(), 1);
    EXPECT_EQ(&target.Get(), observer.lock().get());
}

TEST(TakenTest, MoveAssignmentReleasesTheReplacedObject)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check that move assignment releases the object the target held.");

    bool first_destroyed{false};
    bool second_destroyed{false};
    Taken<DestructionFlag> source{std::make_shared<DestructionFlag>(first_destroyed)};
    Taken<DestructionFlag> target{std::make_shared<DestructionFlag>(second_destroyed)};

    target = std::move(source);

    EXPECT_FALSE(first_destroyed);
    EXPECT_TRUE(second_destroyed);
}

TEST(TakenDeathTest, ConstructionFromNullAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that building a handle from a null pointer aborts.");

    EXPECT_DEATH(Taken<Counter>{std::shared_ptr<Counter>{}}, "");
}

TEST(TakenDeathTest, AccessAfterConversionToBaseAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that access through a handle converted away aborts.");

    DerivedCounter counter{};
    auto source{MakeTaken(counter)};
    const Taken<Counter> target{std::move(source)};

    EXPECT_DEATH(source.Get(), "");
}

TEST(TakenDeathTest, AccessAfterMoveConstructionAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that every access through a moved-from handle aborts.");

    Counter counter{};
    auto source{MakeTaken(counter)};
    const Taken<Counter> target{std::move(source)};

    EXPECT_DEATH(source.Get(), "");
    EXPECT_DEATH(*source, "");
    EXPECT_DEATH(source->value = 1, "");
}

TEST(TakenDeathTest, AccessAfterMoveAssignmentAborts)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that access through a handle moved away by assignment aborts.");

    Counter first{};
    Counter second{};
    auto source{MakeTaken(first)};
    auto target{MakeTaken(second)};
    target = std::move(source);

    EXPECT_DEATH(source.Get(), "");
}

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

#include "score/mw/lifecycle/aasapplicationcontainer.h"
#include "score/mw/lifecycle/mock_application.h"
#include "score/mw/lifecycle/mock_lifecycle_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

namespace score::mw::lifecycle
{

namespace
{

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;

/// @brief Test helper that wraps MockApplication with configurable return values.
/// @note During test we don't have access to the internal application instances,
/// so we configure return values in advance, as part of this constructor to set expectations.
class ConfigurableMockApplication : public ::testing::NiceMock<MockApplication>
{
  public:
    ConfigurableMockApplication(std::int32_t initialize_result, std::int32_t run_result)
    {
        ON_CALL(*this, Initialize(_)).WillByDefault(Return(initialize_result));
        ON_CALL(*this, Run(_)).WillByDefault(Return(run_result));
    }
};

/// @brief A test fixture for AasApplicationContainer
class AasApplicationContainerTest : public ::testing::Test
{
  public:
    static constexpr std::int32_t SUCCESS_CODE = 0;
    static constexpr std::int32_t FAILURE_CODE = 42;
    static constexpr std::size_t ZERO_APPLICATIONS = 0U;
    static constexpr std::int32_t DEFAULT_NUM_ARGS = 1;
    static constexpr const char* DEFAULT_ARGS[DEFAULT_NUM_ARGS] = {"test_app"};

  protected:
    score::mw::lifecycle::LifeCycleManagerMock lifecycle_manager_mock_;
};

using ContainerTest = AasApplicationContainerTest;

TEST_F(ContainerTest, GivenContainerMinSize_WhenInitializeCalled)
{
    // An empty container represents the lowest boundary for the number of applications
    // The upper boundary is the maximum value that can be stored in std::size_t, this can be
    // calculated using MAX_SIZE = (size_t) - 1;
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given an empty container. "
        "When Initialize() is called on the container. "
        "Then Initialize() returns 0 immediately.");

    // Given
    AasApplicationContainer container{
        ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, ContainerTest::ZERO_APPLICATIONS};
    const ApplicationContext context{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS};

    // When
    const auto result = container.Initialize(context);

    // Then
    EXPECT_EQ(result, ContainerTest::SUCCESS_CODE);
}

TEST_F(ContainerTest, GivenContainerMaxSize_WhenConstructed)
{
    // The upper boundary is the maximum value that can be stored in std::size_t, this can be
    // calculated using MAX_SIZE = (size_t) - 1;
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a container of max size ((size_t) - 1). "
        "When the container is constructed. "
        "Then construction throws a std::length_error.");

    // Given
    const std::size_t MAX_SIZE = (size_t)-1;

    // When, Then
    EXPECT_THROW(
        (AasApplicationContainer{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, MAX_SIZE}),
        std::length_error);
}

TEST_F(ContainerTest, GivenOneMoreAppsThanExpected_WhenWithCalled_ThenAssert)
{
    // An equivalence class exists for:
    // - any number of additional applications above the number of allocated applications
    // Where this will cause an assertion to fail
    // This test handles the minimum number of additional applications: 1
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a single application allocated in a container. "
        "Expect all applications to Initialize successfully. "
        "When two applications are added to the container using the With() call. "
        "Then With() asserts");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 1;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};

    // When, Then
    ASSERT_DEATH(
        {
            container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
                .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
        },
        "Passed more Applications than expected");
}

TEST_F(ContainerTest, GivenMinAllocatableApps_ExpectAppInitSuccess_WhenContainerInitializeCalled)
{
    // An equivalence class exists for:
    // - any number of applications in a container that return success,
    // - where the container is allocatable
    //
    // This test handles the minimum number of successful applications as a boundary value of this equivalence class
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a single application configured in a container. "
        "Expect all applications to Initialize successfully. "
        "When Initialize() is called on the container. "
        "Then Initialize() returns success");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 1;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    const ApplicationContext context{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS};

    // When
    const auto result = container.Initialize(context);

    // Then
    EXPECT_EQ(result, ContainerTest::SUCCESS_CODE);
}

TEST_F(ContainerTest, GivenMaxAllocatableApps_ExpectAppInitSuccess_WhenContainerInitializeCalled)
{
    // An equivalence class exists for:
    // - any number of applications in a container that return success,
    // - where the container is allocatable
    // To ensure the container is allocatable we are restricting the container size to 10 elements for this boundary
    // test
    //
    // This test handles the maximum number of successful applications as a boundary value of this equivalence class
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given multiple applications configured in an allocatable container. "
        "Expect all applications to Initialize successfully. "
        "When Initialize() is called on the container. "
        "Then Initialize() returns success");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 10;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    const ApplicationContext context{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS};

    // When
    const auto result = container.Initialize(context);

    // Then
    EXPECT_EQ(result, ContainerTest::SUCCESS_CODE);
}

TEST_F(ContainerTest, GivenMultipleAllocatableApps_ExpectOneInitializeFailure_WhenInitializeCalled)
{
    // An equivalence class exists for:
    // - any number (>0) of failing applications within an allocatable container
    // - Where the number of applications does not exceed the allocated size
    // This equivalence class will always return the failure code from the first failure
    // To simplify testing we are limiting the upper boundary to 10
    // This boundary test handles the minimum number of failing applications: 1
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given multiple allocatable applications in a container. "
        "Expect one application to fail to Initialize. "
        "When Initialize() is called on the container. "
        "Then Initialize() returns the error code from the failed application Initialize() call.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::FAILURE_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    const ApplicationContext context{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS};

    // When
    const auto result = container.Initialize(context);

    // Then
    EXPECT_EQ(result, ContainerTest::FAILURE_CODE);
}

TEST_F(ContainerTest, GivenMultipleAllocatableApps_ExpectMultipleInitializeFailures_WhenInitializeCalled)
{
    // An equivalence class exists for:
    // - any number (>0) of failing applications within an allocatable container
    // - Where the number of applications does not exceed the allocated size
    // This equivalence class will always return the failure code from the first failure
    // This boundary test handles the maximum number of failing applications:
    // 3 (where allocated applications in container is 3)
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given multiple allocatable applications in a container. "
        "Expect all applications to fail to Initialize with unique failure codes. "
        "When Initialize() is called on the container. "
        "Then Initialize() returns the error code from the first failed application Initialize() call.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::FAILURE_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::FAILURE_CODE + 1, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::FAILURE_CODE + 2, ContainerTest::SUCCESS_CODE);
    const ApplicationContext context{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS};

    // When
    const auto result = container.Initialize(context);

    // Then
    EXPECT_EQ(result, ContainerTest::FAILURE_CODE);
}

TEST_F(ContainerTest, GivenEmptyContainer_WhenRunCalled)
{
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given an empty container. "
        "When Run() is called on the container. "
        "Then Run() returns success.");

    // Given
    AasApplicationContainer container{
        ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, ContainerTest::ZERO_APPLICATIONS};
    score::cpp::stop_source stop_source{};

    // When
    const auto result = container.Run(stop_source.get_token());

    // Then
    EXPECT_EQ(result, ContainerTest::SUCCESS_CODE);
}

TEST_F(ContainerTest, GivenMultipleAllocatableApps_ExpectOneAppRunFailure_WhenRunCalled)
{
    // An equivalence class exists for:
    // - any number (>0) of failing applications within an allocatable container
    // - Where the number of applications does not exceed the allocated size
    // To simplify testing we are limiting the upper boundary to 10
    // This boundary test handles the minimum number of failing applications: 1
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given multiple allocatable applications in a container. "
        "Expect one application to fail to Run(). "
        "When Run() is called on the container. "
        "Then Run() returns the error code from the failed application Run() call.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    score::cpp::stop_source stop_source{};

    // When
    const auto result = container.Run(stop_source.get_token());

    // Then
    EXPECT_EQ(result, ContainerTest::FAILURE_CODE);
}

TEST_F(ContainerTest, GivenMultipleAllocatableApps_ExpectAllAppRunCallsFail_WhenRunCalled)
{
    // An equivalence class exists for:
    // - any number (>0) of failing applications within an allocatable container
    // - Where the number of applications does not exceed the allocated size
    // This equivalence class will always return the failure code from the last failure
    // This boundary test handles the maximum number of failing applications:
    // 3 (where allocated applications in container is 3)
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given multiple allocatable applications in a container. "
        "Expect all applications to fail to Run() with unique failure codes. "
        "When Run() is called on the container. "
        "Then Run() returns the error code from the last failed application Run() call.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE + 1)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE + 2);
    score::cpp::stop_source stop_source{};

    // When
    const auto result = container.Run(stop_source.get_token());

    // Then
    EXPECT_EQ(result, ContainerTest::FAILURE_CODE + 2);
}

TEST_F(ContainerTest, GivenMultipleAllocatableApps_ExpectAppRunFailsThenLastSucceeds_WhenRunCalled)
{
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Given multiple applications in a container. "
        "Expect multiple applications to fail with unique error codes, and the last application to succeed when Run() "
        "called. "
        "When Run() is called on the container. "
        "Then Run() returns the error code from the first failed application Run() call.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::FAILURE_CODE + 1)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    score::cpp::stop_source stop_source{};

    // When
    const auto result = container.Run(stop_source.get_token());

    // Then
    EXPECT_EQ(result, ContainerTest::FAILURE_CODE);
}

TEST_F(ContainerTest, GivenMultipleApps_ExpectAllAppRunSuccess_WhenContainerRunCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given multiple applications configured in a container. "
        "Expect all applications to succeed during Initialize() and Run(). "
        "When the container's Run() method is called. "
        "Then the container's Run() method returns success.");

    // Given, Expect
    constexpr std::size_t CONTAINER_SIZE = 3U;
    AasApplicationContainer container{ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, CONTAINER_SIZE};
    container.With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE)
        .With<ConfigurableMockApplication>(ContainerTest::SUCCESS_CODE, ContainerTest::SUCCESS_CODE);
    score::cpp::stop_source stop_source{};

    // When
    const auto result = container.Run(stop_source.get_token());

    // Then
    EXPECT_EQ(result, ContainerTest::SUCCESS_CODE);
}

TEST_F(ContainerTest, GivenEmptyContainer_ExpectRunCalledWithContainerAsApp_WhenLaunchCalled)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a container with no applications. "
        "Expect LifecycleManager::run() to be called with the container as the Application parameter. "
        "When Launch() is called on the container. "
        "Then Launch() returns the value returned from LifecycleManager::run().");

    // Given
    AasApplicationContainer container{
        ContainerTest::DEFAULT_NUM_ARGS, ContainerTest::DEFAULT_ARGS, ContainerTest::ZERO_APPLICATIONS};

    // Expect
    const std::int32_t EXPECTED_RETURN_VALUE = 42;
    EXPECT_CALL(lifecycle_manager_mock_, run(_, _)).WillOnce([&container](Application& app, const ApplicationContext&) {
        EXPECT_EQ(&app, &container);
        return EXPECTED_RETURN_VALUE;
    });

    // When
    const auto result = container.Launch();

    // Then
    EXPECT_EQ(result, EXPECTED_RETURN_VALUE);
}

}  // namespace

}  // namespace score::mw::lifecycle

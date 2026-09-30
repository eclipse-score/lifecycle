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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/mw/launch_manager/alive_monitor/details/factory/AliveWorkerFactory.hpp"

using namespace testing;

namespace score::mw::lifecycle::internal::saf::factory
{

struct ConstructionException
{
    std::string_view reason;
};

class MockObservable : public common::Observable<MockObservable>
{
  public:
    void PushResultToObservers()
    {
        pushResultToObservers(*this);
    }

    MOCK_METHOD(void, attachObserver, (common::Observer<MockObservable> & f_observer_r), (noexcept(false)));
    MOCK_METHOD(void, detachObserver, (common::Observer<MockObservable> & f_observer_r), (noexcept(false)));
};

class MockObserver : public common::Observer<MockObservable>
{
    MOCK_METHOD(void, updateData, (const MockObservable& f_observable_r), (noexcept));
    MOCK_METHOD(IdentifierHash, getIdentifier, (), (const, noexcept));

  public:
    MockObserver(bool throw_something, int data) : data_(data)
    {
        if (throw_something)
        {
            Throw(ConstructionException{"Construction error"});
        }
    }

    int data_;
};

class AliveWorkerFactoryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    AliveWorkerFactory factory{};
};

class EmplaceAndAttachTest : public AliveWorkerFactoryTest
{
  protected:
    std::vector<MockObserver> observers{};
    MockObservable mock_observable{};

    const int emplaced_data = 42;
};

TEST_F(EmplaceAndAttachTest, EmplaceAndAttachSuccess)
{
    RecordProperty("Description", "Test that EmplaceAndAttach emplaces and attaches the correct data on a success");

    // EXPECT_CALL(mock_observable, attachObserver(Field(&MockObserver::data_, emplaced_data)));
}

}  // namespace score::mw::lifecycle::internal::saf::factory

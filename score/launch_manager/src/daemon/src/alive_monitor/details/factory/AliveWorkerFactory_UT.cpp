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
#include "score/mw/launch_manager/alive_monitor/details/ifexm/MockObservableEventReader.hpp"

using namespace testing;

namespace score::mw::lifecycle::internal::saf::factory
{

// struct ConstructionException
// {
//     std::string_view reason;
// };

// class MockObservable : public common::Observable<MockObservable>
// {
//   public:
//     void PushResultToObservers()
//     {
//         pushResultToObservers(*this);
//     }

//     MOCK_METHOD(void, attachObserver, (common::Observer<MockObservable> & f_observer_r), (noexcept(false)));
//     MOCK_METHOD(void, detachObserver, (common::Observer<MockObservable> & f_observer_r), (noexcept(false)));
// };

// class MockObserver : public common::Observer<MockObservable>
// {
//     MOCK_METHOD(void, updateData, (const MockObservable& f_observable_r), (noexcept));
//     MOCK_METHOD(IdentifierHash, getIdentifier, (), (const, noexcept));

//   public:
//     MockObserver(bool throw_something, int data) : data_(data)
//     {
//         if (throw_something)
//         {
//             Throw(ConstructionException{"Construction error"});
//         }
//     }

//     int data_;
// };

struct FactoryException : public std::exception
{
    explicit FactoryException(std::string_view reason) : reason_(reason) {};

    std::string_view reason_;

    const char* what() const noexcept override
    {
        return reason_.data();
    }
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

// class EmplaceAndAttachTest : public AliveWorkerFactoryTest
// {
//   protected:
//     std::vector<MockObserver> observers{};
//     MockObservable mock_observable{};

//     const int emplaced_data = 42;
// };

// TEST_F(EmplaceAndAttachTest, EmplaceAndAttachSuccess)
// {
//     RecordProperty("Description", "Test that EmplaceAndAttach emplaces and attaches the correct data on a success");

//     EXPECT_CALL(mock_observable, attachObserver(Field(&MockObserver::data_, emplaced_data)));
// }

TEST_F(AliveWorkerFactoryTest, createObservableEventFails)
{
    RecordProperty(
        "Description",
        "Verify that exceptions thrown while creating an observable event are caught and a failure is returned");

    ifexm::MockObservableEventReader reader{};
    IdentifierHash event_id{"Observable"};
    std::vector<ifexm::ObservableEvent> out{};
    bool create_result = true;

    EXPECT_CALL(reader, registerObservableEvent).WillOnce(Throw(FactoryException{"Exception during registration!"}));

    EXPECT_NO_THROW(create_result = factory.createObservableEvent(out, event_id, reader));
    EXPECT_FALSE(create_result);
}

TEST_F(AliveWorkerFactoryTest, createObservableEventSucceeds)
{
    RecordProperty(
        "Description",
        "Verify that createObservableEvent creates an attaches an observable event with the correct information");

    ifexm::MockObservableEventReader reader{};
    IdentifierHash event_id{"Observable"};
    std::vector<ifexm::ObservableEvent> out{};
    bool create_result = false;
    PolymorphicMatcher event_matcher =
        Field(&ifexm::ObservableEvent::event, Field(&SupervisionEvent::id, Eq(event_id)));

    EXPECT_CALL(reader, registerObservableEvent(event_matcher, event_id)).WillOnce(Return(true));

    create_result = factory.createObservableEvent(out, event_id, reader);

    EXPECT_TRUE(create_result);
    EXPECT_THAT(out.back(), event_matcher);
}

}  // namespace score::mw::lifecycle::internal::saf::factory

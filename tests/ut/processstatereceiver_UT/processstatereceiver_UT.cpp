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
#include <score/lcm/processstatereceiver.hpp>
#include <gtest/gtest.h>

using namespace testing;
using namespace score::lcm;

// To do

class ProcessStateReceiver_UT : public ::testing::Test {
    public:
};

TEST_F(ProcessStateReceiver_UT, Smoke) {
    auto buffer = std::make_shared<ipc_dropin::RingBuffer<static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE), static_cast<size_t>(BufferConstants::BUFFER_MAXPAYLOAD)>>();
    auto psr = ProcessStateReceiver(buffer);
    SUCCEED();
}
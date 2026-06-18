/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"

namespace score {
namespace lcm {

class RecoveryClientTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

TEST_F(RecoveryClientTest, SendSingleRequest)
{
    RecordProperty("Description",
                   "RecoveryClient can send single request successfully.");

    RecoveryClient client;
    const bool result = client.sendRecoveryRequest(IdentifierHash("proc_a"));
    EXPECT_TRUE(result);
    EXPECT_FALSE(client.hasOverflow());
}

TEST_F(RecoveryClientTest, GetNextRequest)
{
    RecordProperty("Description",
                   "RecoveryClient can send and retrieve single request successfully.");

    RecoveryClient client;
    const IdentifierHash expected_proc("proc_b");
    client.sendRecoveryRequest(expected_proc);

    const auto req = client.getNextRequest();
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(*req, expected_proc);
}

TEST_F(RecoveryClientTest, GetNextRequestEmpty)
{
    RecordProperty("Description",
                   "RecoveryClient returns no request when buffer is empty.");

    RecoveryClient client;
    EXPECT_FALSE(client.getNextRequest().has_value());
}

TEST_F(RecoveryClientTest, RingBufferFull)
{
    RecordProperty("Description",
                   "RecoveryClient sets overflow flag if buffer is full.");

    RecoveryClient client;
    const IdentifierHash proc("proc_c");

    // Fill the ring buffer
    for (std::size_t i = 0U; i < RecoveryClient::kBufferCapacity; ++i)
    {
        EXPECT_TRUE(client.sendRecoveryRequest(proc));
    }

    // One more should fail and set overflow
    EXPECT_FALSE(client.sendRecoveryRequest(proc));
    EXPECT_TRUE(client.hasOverflow());
}

TEST_F(RecoveryClientTest, FIFOOrdering)
{
    RecordProperty("Description",
                   "RecoveryClient maintains the order of inserted requests");

    RecoveryClient client;
    const IdentifierHash proc_first("proc_first");
    const IdentifierHash proc_second("proc_second");
    const IdentifierHash proc_third("proc_third");

    client.sendRecoveryRequest(proc_first);
    client.sendRecoveryRequest(proc_second);
    client.sendRecoveryRequest(proc_third);

    const auto req1 = client.getNextRequest();
    ASSERT_TRUE(req1.has_value());
    EXPECT_EQ(req1.value(), proc_first);

    const auto req2 = client.getNextRequest();
    ASSERT_TRUE(req2.has_value());
    EXPECT_EQ(req2.value(), proc_second);

    const auto req3 = client.getNextRequest();
    ASSERT_TRUE(req3.has_value());
    EXPECT_EQ(req3.value(), proc_third);

    EXPECT_FALSE(client.getNextRequest().has_value());
}

} // namespace lcm
} // namespace score

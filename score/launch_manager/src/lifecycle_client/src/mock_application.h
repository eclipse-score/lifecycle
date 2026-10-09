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

#ifndef SCORE_MW_LIFECYCLE_MOCK_APPLICATION_H_
#define SCORE_MW_LIFECYCLE_MOCK_APPLICATION_H_

#include "score/mw/lifecycle/application.h"
#include <gmock/gmock.h>

namespace score::mw::lifecycle
{

class MockApplication : public Application
{
  public:
    ~MockApplication() override = default;
    MOCK_METHOD(std::int32_t, Initialize, (const ApplicationContext&), (override));
    MOCK_METHOD(std::int32_t, Run, (const score::cpp::stop_token&), (override));
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_MOCK_APPLICATION_H_

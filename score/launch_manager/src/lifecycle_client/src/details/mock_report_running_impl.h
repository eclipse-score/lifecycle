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

#ifndef SCORE_MW_LIFECYCLE_MOCKS_REPORTRUNNINGIMPLMOCK_H_
#define SCORE_MW_LIFECYCLE_MOCKS_REPORTRUNNINGIMPLMOCK_H_

#include "score/result/result.h"
#include <gmock/gmock.h>
#include <variant>

namespace score::mw::lifecycle
{

class ReportRunningImplMock
{
  public:
    ReportRunningImplMock();
    ~ReportRunningImplMock();

    MOCK_METHOD(score::Result<std::monostate>, ReportRunningState, (), (const, noexcept));
    MOCK_METHOD(void, ctor, (), ());
    MOCK_METHOD(void, dtor, (), ());

    void ResetReportedFlag();
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_MOCKS_REPORTRUNNINGIMPLMOCK_H_

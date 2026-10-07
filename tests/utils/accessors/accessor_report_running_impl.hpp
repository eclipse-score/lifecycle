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
#ifndef ACCESSOR_REPORT_RUNNING_IMPL_HPP
#define ACCESSOR_REPORT_RUNNING_IMPL_HPP

#include "score/mw/lifecycle/lifecycle_client/details/report_running_impl.hpp"

namespace score::mw::lifecycle
{

class ReportRunningImplTestAccessor
{
  public:
    /// @brief Mutator method to set reported value
    /// @note For testing purposes only
    static void SetReportedForTesting(bool value)
    {
        ReportRunningImpl::reported = value;
    }

    /// @brief Accessor method to get reported value
    /// @note For testing purposes only
    static bool GetReportedForTesting()
    {
        return ReportRunningImpl::reported;
    }
};
}  // namespace score::mw::lifecycle

#endif  // ACCESSOR_REPORT_RUNNING_IMPL_HPP

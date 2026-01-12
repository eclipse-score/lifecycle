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


#ifndef SCORE_LCM_IHEALTH_MONITOR_HPP_INCLUDED
#define SCORE_LCM_IHEALTH_MONITOR_HPP_INCLUDED

namespace score
{
namespace lcm
{
namespace internal
{
class IHealthMonitorThread {
 public:
    virtual bool start() = 0;
    virtual void stop() = 0;
};
}
}
}

#endif
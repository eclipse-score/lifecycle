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

#ifndef SCORE_LCM_ISTART_ACTION_HPP_INCLUDED
#define SCORE_LCM_ISTART_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/handle.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

class IStartAction
{
  public:
    virtual ~IStartAction() = default;
    virtual Result<Handle> start() const = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ISTART_ACTION_HPP_INCLUDED

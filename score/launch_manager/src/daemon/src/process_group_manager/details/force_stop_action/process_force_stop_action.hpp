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

#ifndef SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED
#define SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/iforce_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

class ProcessForceStopAction final : public IForceStopAction
{
  public:
    explicit ProcessForceStopAction(osal::IProcess& launcher);

    Result<void> force_stop(const Handle handle) const override;
    Result<void> operator()(const ProcessHandle process) const;
    Result<void> operator()(const Handle handle) const;

  private:
    osal::IProcess& launcher_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_FORCE_STOP_ACTION_HPP_INCLUDED

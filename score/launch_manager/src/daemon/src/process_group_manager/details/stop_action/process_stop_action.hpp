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

#ifndef SCORE_LCM_PROCESS_STOP_ACTION_HPP_INCLUDED
#define SCORE_LCM_PROCESS_STOP_ACTION_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

class ProcessStopAction final : public IStopAction
{
  public:
    explicit ProcessStopAction(osal::ProcessLauncher& launcher);

    Result<void> stop(const Handle handle) const override;
    Result<void> operator()(const ProcessHandle process) const;
    Result<void> operator()(const Handle handle) const;

  private:
    osal::ProcessLauncher& launcher_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_PROCESS_STOP_ACTION_HPP_INCLUDED

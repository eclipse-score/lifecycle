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

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/empty_force_stop_action.hpp"

namespace score::mw::lifecycle::internal
{

Result<IComponent::RequestState> EmptyForceStopAction::forceStop(cpp::stop_token stop_token, const Handle) const
{
    return IComponent::RequestState::kSuccess;
}

}  // namespace score::mw::lifecycle::internal

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

#ifndef SCORE_LCM_COMPONENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"
#include <vector>

namespace score::mw::lifecycle::internal
{

class Component final : public IComponent
{
  public:
    Component(
        const IStartAction* start_action,
        const IStopAction* stop_action,
        const std::vector<const IReadyCondition*> ready_conditions,
        IdentifierHash identifier = IdentifierHash{});

    RequestResult activate(score::cpp::stop_token stop_token) override;
    RequestResult deactivate(score::cpp::stop_token stop_token) override;
    RequestResult tryHandleTermination(int32_t status) override;
    [[nodiscard]] IdentifierHash getIdentifier() const override;
    [[nodiscard]] bool active() const override;

  private:
    std::optional<Handle> handle_;
    const IStartAction* start_action_;
    const IStopAction* stop_action_;
    const std::vector<const IReadyCondition*> ready_conditions_;
    IdentifierHash identifier_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_HPP_INCLUDED

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
#ifndef SCORE_LCM_COMPONENT_OF_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_OF_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/run_target.hpp"

#include <memory>
#include <type_traits>
#include <variant>

namespace score::mw::lifecycle::internal
{

/// @brief Returns the IComponent reference from a variant type
/// @details All types in the variant must implement the IComponent interface. A Component is held by
///          pointer, because it cannot be moved.
inline IComponent& componentOf(std::variant<std::unique_ptr<Component>, RunTarget>& node)
{
    return std::visit(
        [](auto& component) -> IComponent& {
            using ComponentT = std::decay_t<decltype(component)>;
            if constexpr (std::is_same_v<ComponentT, std::unique_ptr<Component>>)
            {
                return *component;
            }
            else
            {
                return component;
            }
        },
        node);
}

/// @brief Returns the IComponent reference from an interface pointer. Useful for a template class that may take a
/// variant or a generic interface
inline IComponent& componentOf(IComponent* node)
{
    return *node;
}

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_OF_HPP_INCLUDED

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

#ifndef SCORE_LCM_ICOMPONENT_HPP_INCLUDED
#define SCORE_LCM_ICOMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <score/stop_token.hpp>
#include <cstdint>

namespace score::mw::lifecycle::internal
{

/// @brief A class that implements IComponent is a node in the configuration that can be activated or deactivated.
///
/// @details Each component type defines the implementation of how it shall reach the active and inactive states.
///          When a component becomes active, components that depend on it may be activated. When a component becomes
///          inactive, components it depends on may be deactivated.
///          For example, for a process implementation of IComponent, @c activate() could involve starting the process
///          and @c deactivate() could terminate the process.
class IComponent
{
  public:
    enum class [[nodiscard]] RequestResult
    {
        /// @brief The operation succeeded.
        kSuccess,

        /// @brief The operation timed out.
        kTimeout,

        /// @brief The operation failed.
        kFailure
    };

    enum class [[nodiscard]] TerminationResult
    {
        /// @brief The termination does not affect the state of the component.
        kIgnore,

        /// @brief The component failed.
        kFailure
    };

    /// @brief Begin activation of the component.
    /// @p stop_token Token that can be stopped to exit the activation early.
    virtual RequestResult activate(score::cpp::stop_token stop_token) = 0;

    /// @brief Begin deactivation of the component.
    /// @p stop_token Token that can be stopped to exit the deactivation early.
    virtual RequestResult deactivate(score::cpp::stop_token stop_token) = 0;

    /// @brief Notify the component that it has terminated with status @p status
    virtual TerminationResult tryHandleTermination(int32_t status) = 0;

    /// @returns the index of the component in the graph.
    [[nodiscard]] virtual IdentifierHash getIdentifier() const = 0;

    /// @returns True if the component is active in the active Run Target.
    [[nodiscard]] virtual bool active() const = 0;

    virtual ~IComponent() = default;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICOMPONENT_HPP_INCLUDED

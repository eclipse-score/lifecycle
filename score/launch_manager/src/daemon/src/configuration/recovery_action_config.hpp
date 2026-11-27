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
#ifndef RECOVERY_ACTION_CONFIG_HPP
#define RECOVERY_ACTION_CONFIG_HPP

#include <cstdint>
#include <string>
#include <variant>

namespace score::mw::lifecycle::internal::configuration
{

/// @brief Who executes a recovery action.
enum class RecoveryActionScope : std::uint8_t
{
    /// @brief Executed by the failed component itself, see IComponent::recover().
    kLocal,

    /// @brief Executed outside of the component, e.g. by switching the Run Target.
    kGlobal
};

/// @brief Switch to another Run Target.
struct SwitchRunTargetAction
{
    static constexpr RecoveryActionScope scope{RecoveryActionScope::kGlobal};

    std::string run_target;
};

/// @brief Restart the failed component.
struct RestartAction
{
    static constexpr RecoveryActionScope scope{RecoveryActionScope::kLocal};
};

/// @brief A recovery action. Every alternative must declare its scope as a `static constexpr` member `scope`.
using RecoveryAction = std::variant<SwitchRunTargetAction, RestartAction>;

/// @brief Return who executes @p action.
/// @param action The recovery action.
/// @return The scope of the alternative held by @p action.
inline RecoveryActionScope getScope(const RecoveryAction& action)
{
    return std::visit(
        [](const auto& alternative) {
            return alternative.scope;
        },
        action);
}

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // RECOVERY_ACTION_CONFIG_HPP

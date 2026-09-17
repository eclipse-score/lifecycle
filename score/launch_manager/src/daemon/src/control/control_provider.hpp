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

#ifndef SCORE_LCM_CONTROL_PROVIDER
#define SCORE_LCM_CONTROL_PROVIDER

#include "score/mw/launch_manager/process_group_manager/irun_target_control.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

#include <memory>

namespace score::mw::lifecycle::internal
{

/// @brief Provides the mw::com service for state managers to connect to.
class ControlProvider
{
  public:
    /// @brief Fallible constructor for ControllableGraph.
    static Result<ControlProvider> Create(IRunTargetControl* graph) noexcept;

    // Movable: the mw::com/graph callbacks registered during `Create` capture a pointer to
    // `Impl`, not to `ControlProvider` itself, so moving a `ControlProvider` only moves the
    // `unique_ptr` — `Impl`'s address (the thing the callbacks actually point at) never
    // changes. Declared out-of-line (not `= default` here) because `Impl` is still an
    // incomplete type at this point; defined in the .cpp once `Impl` is complete.
    ControlProvider(ControlProvider&&) noexcept;
    ControlProvider& operator=(ControlProvider&&) noexcept;
    ~ControlProvider();

    ControlProvider(const ControlProvider&) = delete;
    ControlProvider& operator=(const ControlProvider&) = delete;

  private:
    class Impl;

    explicit ControlProvider(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER

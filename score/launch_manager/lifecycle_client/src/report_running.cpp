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

#include "score/mw/lifecycle/lifecycle_client/report_running.h"

#include "score/mw/log/logging.h"
#include <score/mw/lifecycle/lifecycle_client.h>

void score::mw::lifecycle::report_running() noexcept {
  static_cast<void>(
      score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(
          score::mw::lifecycle::ExecutionState::kRunning));
}
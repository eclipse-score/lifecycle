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

#ifndef SCORE_MW_LIFECYCLE_REPORT_RUNNING_H_
#define SCORE_MW_LIFECYCLE_REPORT_RUNNING_H_

#include <stdint.h>

#ifdef __cplusplus

namespace score
{
namespace mw
{
namespace lifecycle
{

/// @brief Signals to the Launch Manager that this process has finished
/// initialization and is now entering its Running state.
///
/// This function provides an alternative API to the @ref Application class, in
/// which case @ref LifecycleManager internally calls report_running() before
/// invoking the Run() method of the application. Clients may use either
/// report_running() directly or implement the @ref Application class, but not
/// both at the same time.
///
/// @note When using report_running() directly instead of the @ref
/// LifecycleManager class, the caller is responsible for handling process
/// shutdown. The @ref LifecycleManager class automatically registers a signal
/// handler for SIGTERM, which is the signal sent by the Launch Manager to
/// request shutdown of the process. When using report_running() directly, users
/// must register their own SIGTERM signal handler and perform a graceful
/// shutdown accordingly.
void report_running() noexcept;

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

extern "C" {
#endif /* __cplusplus */

// This prototype is required to support applications written in C, which cannot use the C++ namespaces and classes.

/// @brief Signals to the Launch Manager that this process has finished initialization and is now running.
/// @return 0 on success, -1 on failure
int8_t score_mw_lifecycle_report_running(void);

#ifdef __cplusplus
}
#endif

#endif /* SCORE_MW_LIFECYCLE_REPORT_RUNNING_H_ */

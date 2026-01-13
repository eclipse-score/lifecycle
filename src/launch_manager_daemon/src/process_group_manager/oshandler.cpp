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

#include <process_group_manager/oshandler.hpp>

namespace score {

namespace lcm {

namespace internal {

void OsHandler::run(void) {
    while (is_running_) {
        int32_t status = 0;
        osal::ProcessID targetProcessId = 0;

        if (osal::OsalReturnType::kSuccess == process_interface_.waitForTermination(targetProcessId, status)) {
            if (-1 == safe_process_map_.findTerminated(targetProcessId, status)) {
                LM_LOG_ERROR() << "[os handler: out of resources]";
            }
        } else {
            // This process has no children to wait for at present,
            // or the wait was interrupted by a signal.
            // Wait a while to stop this thread from hogging cpu time
            std::this_thread::sleep_for(OS_HANDLER_LOOP_DELAY);
        }
    }
}

}  // namespace lcm

}  // namespace internal

}  // namespace score

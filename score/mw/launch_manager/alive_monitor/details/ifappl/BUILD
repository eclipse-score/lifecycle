# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "data_structures",
    hdrs = ["DataStructures.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/ipc:ipc_if",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "checkpoint",
    srcs = ["Checkpoint.cpp"],
    hdrs = ["Checkpoint.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/common:observer",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "monitor_if_daemon",
    srcs = ["MonitorIfDaemon.cpp"],
    hdrs = ["MonitorIfDaemon.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":checkpoint",
        ":data_structures",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
    ],
)

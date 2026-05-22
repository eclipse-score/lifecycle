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
    name = "process_cfg",
    srcs = ["ProcessCfg.cpp"],
    hdrs = ["ProcessCfg.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/common:types",
    ],
)

cc_library(
    name = "process_state",
    srcs = ["ProcessState.cpp"],
    hdrs = ["ProcessState.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":process_cfg",
        "//score/mw/launch_manager/alive_monitor/details/common:observer",
        "//score/mw/launch_manager/alive_monitor/details/common:types",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/process_state_client",
    ],
)

cc_library(
    name = "process_state_reader",
    srcs = ["ProcessStateReader.cpp"],
    hdrs = ["ProcessStateReader.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":process_state",
        "//score/mw/launch_manager/alive_monitor/details/common:types",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/timers:time_conversion",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/process_state_client",
    ],
)

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
    name = "monitor_impl",
    srcs = ["MonitorImpl.cpp"],
    hdrs = ["MonitorImpl.h"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor:alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor:config",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifappl:data_structures",
        "//score/launch_manager/daemon/src/alive_monitor/details/ipc:ipc_if",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "@flatbuffers",
    ],
)

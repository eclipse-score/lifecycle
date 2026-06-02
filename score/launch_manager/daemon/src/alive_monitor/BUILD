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

filegroup(
    name = "am_flatcfg_fbs",
    srcs = ["config/hm_flatcfg.fbs"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "am_core_flatcfg_fbs",
    srcs = ["config/hmcore_flatcfg.fbs"],
    visibility = ["//visibility:public"],
)

exports_files(["config/hm_flatcfg.fbs"])

cc_library(
    name = "config",
    hdrs = [
        "config/hm_flatcfg_generated.h",
        "config/hmcore_flatcfg_generated.h",
    ],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "alive_monitor_h",
    hdrs = [
        "Monitor.h",
        "MonitorImplWrapper.h",
    ],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "am-lib",
    srcs = [
        "Monitor.cpp",
        "MonitorImplWrapper.cpp",
    ],
    hdrs = [
        "Monitor.h",
        "MonitorImplWrapper.h",
    ],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor/details:monitor_impl",
    ] + select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["//externals/acl"],
    }),
)

cc_library(
    name = "alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor/details/daemon:health_monitor_impl",
    ],
)

cc_library(
    name = "am_shared_lib",
    hdrs = [
        "Monitor.h",
        "MonitorImplWrapper.h",
    ],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":am-lib",
        "//score/launch_manager/daemon/src/alive_monitor/details/ipc:ipc_if",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "@flatbuffers",
    ],
    alwayslink = True,
)

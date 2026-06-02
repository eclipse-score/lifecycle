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
    name = "supervision_cfg",
    hdrs = ["SupervisionCfg.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor/details/common:types",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "i_supervision",
    srcs = ["ISupervision.cpp"],
    hdrs = ["ISupervision.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "process_state_tracker",
    srcs = ["ProcessStateTracker.cpp"],
    hdrs = ["ProcessStateTracker.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor/details/common:types",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
    ],
)

cc_library(
    name = "i_checkpoint_supervision",
    srcs = ["ICheckpointSupervision.cpp"],
    hdrs = ["ICheckpointSupervision.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":i_supervision",
        ":process_state_tracker",
        ":supervision_cfg",
        "//score/launch_manager/daemon/src/alive_monitor:alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifexm:process_state",
    ],
)

cc_library(
    name = "alive",
    srcs = ["Alive.cpp"],
    hdrs = ["Alive.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":i_checkpoint_supervision",
        ":process_state_tracker",
        ":supervision_cfg",
        "//score/launch_manager/daemon/src/alive_monitor:alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:time_sorting_buffer",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "local",
    srcs = ["Local.cpp"],
    hdrs = ["Local.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":alive",
        ":i_checkpoint_supervision",
        ":i_supervision",
        ":supervision_cfg",
        "//score/launch_manager/daemon/src/alive_monitor:alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:time_sorting_buffer",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "global",
    srcs = ["Global.cpp"],
    hdrs = ["Global.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":i_checkpoint_supervision",
        ":i_supervision",
        ":local",
        ":supervision_cfg",
        "//score/launch_manager/daemon/src/alive_monitor:alive_monitor_h",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:time_sorting_buffer",
        "//score/launch_manager/daemon/src/alive_monitor/details/common:types",
        "//score/launch_manager/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/recovery:notification",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

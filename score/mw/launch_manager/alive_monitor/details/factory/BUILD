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
    name = "i_phm_factory",
    hdrs = ["IPhmFactory.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/ifappl:data_structures",
    ],
)

cc_library(
    name = "static_config",
    hdrs = ["StaticConfig.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/ifappl:data_structures",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/alive_monitor/details/watchdog:i_device_config_factory",
    ],
)

cc_library(
    name = "machine_config_factory",
    srcs = ["MachineConfigFactory.cpp"],
    hdrs = ["MachineConfigFactory.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":static_config",
        "//score/mw/launch_manager/alive_monitor:config",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/timers:time_conversion",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/alive_monitor/details/watchdog:i_device_config_factory",
        "@flatbuffers",
    ] + select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["//externals/acl"],
    }),
)

cc_library(
    name = "flat_cfg_factory",
    srcs = ["FlatCfgFactory.cpp"],
    hdrs = ["FlatCfgFactory.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":i_phm_factory",
        ":machine_config_factory",
        "//score/mw/launch_manager/alive_monitor:config",
        "//score/mw/launch_manager/alive_monitor/details/common:types",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:checkpoint",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state_reader",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/recovery:notification",
        "//score/mw/launch_manager/alive_monitor/details/supervision:alive",
        "//score/mw/launch_manager/alive_monitor/details/supervision:global",
        "//score/mw/launch_manager/alive_monitor/details/supervision:local",
        "//score/mw/launch_manager/alive_monitor/details/supervision:supervision_cfg",
        "//score/mw/launch_manager/alive_monitor/details/timers:time_conversion",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/common:identifier_hash",
        "@flatbuffers",
    ] + select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["//externals/acl"],
    }),
)

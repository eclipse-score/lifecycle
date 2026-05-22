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
    name = "phm_daemon_config",
    hdrs = ["PhmDaemonConfig.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "sw_cluster_handler",
    srcs = ["SwClusterHandler.cpp"],
    hdrs = ["SwClusterHandler.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/factory:flat_cfg_factory",
        "//score/mw/launch_manager/alive_monitor/details/factory:machine_config_factory",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:checkpoint",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:data_structures",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state_reader",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/recovery:notification",
        "//score/mw/launch_manager/alive_monitor/details/supervision:alive",
        "//score/mw/launch_manager/alive_monitor/details/supervision:global",
        "//score/mw/launch_manager/alive_monitor/details/supervision:local",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "phm_daemon",
    srcs = ["PhmDaemon.cpp"],
    hdrs = ["PhmDaemon.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":phm_daemon_config",
        ":sw_cluster_handler",
        "//score/mw/launch_manager/alive_monitor/details/factory:flat_cfg_factory",
        "//score/mw/launch_manager/alive_monitor/details/factory:machine_config_factory",
        "//score/mw/launch_manager/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/mw/launch_manager/alive_monitor/details/ifexm:process_state_reader",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/supervision:alive",
        "//score/mw/launch_manager/alive_monitor/details/supervision:global",
        "//score/mw/launch_manager/alive_monitor/details/supervision:local",
        "//score/mw/launch_manager/alive_monitor/details/timers:cycle_time_validator",
        "//score/mw/launch_manager/alive_monitor/details/timers:cycle_timer",
        "//score/mw/launch_manager/alive_monitor/details/timers:timers_os_clock",
        "//score/mw/launch_manager/alive_monitor/details/watchdog:i_watchdog_if",
        "//score/mw/lifecycle/control_client",
        "//score/mw/lifecycle/lifecycle_client",
    ],
)

cc_library(
    name = "i_health_monitor",
    hdrs = ["IAliveMonitor.hpp"],
    visibility = ["//visibility:public"],
    deps = [":phm_daemon"],
)

cc_library(
    name = "health_monitor_impl",
    srcs = ["AliveMonitorImpl.cpp"],
    hdrs = ["AliveMonitorImpl.hpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":i_health_monitor",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/watchdog:watchdog_impl",
    ],
)

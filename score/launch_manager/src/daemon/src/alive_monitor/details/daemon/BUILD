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
    name = "sw_cluster_handler",
    srcs = ["SupervisionManager.cpp"],
    hdrs = ["SupervisionManager.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:alive_worker_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event_reader",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:alive",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:config",
    ],
)

cc_library(
    name = "cyclic_executor",
    srcs = ["CyclicExecutor.cpp"],
    hdrs = ["CyclicExecutor.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":sw_cluster_handler",
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:einitcode",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:alive_worker_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event_reader",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:alive",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:cycle_time_validator",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:cycle_timer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

cc_library(
    name = "health_monitor_impl",
    srcs = ["AliveMonitorImpl.cpp"],
    hdrs = ["AliveMonitorImpl.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon:__subpackages__"],
    deps = [
        ":cyclic_executor",
        "//score/launch_manager/src/daemon/src/alive_monitor:i_alive_monitor",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "@score_baselibs//score/language/futurecpp",
    ],
)

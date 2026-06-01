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
    name = "i_device_config_factory",
    hdrs = ["IDeviceConfigFactory.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/watchdog",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/watchdog",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "i_watchdog_if",
    hdrs = ["IWatchdogIf.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/watchdog",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/watchdog",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [":i_device_config_factory"],
)

cc_library(
    name = "watchdog",
    hdrs = ["Watchdog.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/watchdog",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/watchdog",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "device_if",
    srcs = ["DeviceIf.cpp"],
    hdrs = ["DeviceIf.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/watchdog",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/watchdog",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "watchdog_impl",
    srcs = ["WatchdogImpl.cpp"],
    hdrs = ["WatchdogImpl.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/watchdog",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/watchdog",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":device_if",
        ":i_device_config_factory",
        ":i_watchdog_if",
        ":watchdog",
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/alive_monitor/details/timers:os_clock_interface",
    ],
)

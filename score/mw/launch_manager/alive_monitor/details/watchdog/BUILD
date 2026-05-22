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
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "i_watchdog_if",
    hdrs = ["IWatchdogIf.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [":i_device_config_factory"],
)

cc_library(
    name = "watchdog",
    hdrs = ["Watchdog.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "device_if",
    srcs = ["DeviceIf.cpp"],
    hdrs = ["DeviceIf.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "watchdog_impl",
    srcs = ["WatchdogImpl.cpp"],
    hdrs = ["WatchdogImpl.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":device_if",
        ":i_device_config_factory",
        ":i_watchdog_if",
        ":watchdog",
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/alive_monitor/details/timers:os_clock_interface",
    ],
)

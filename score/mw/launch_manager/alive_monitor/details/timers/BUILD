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
    name = "os_clock_interface",
    hdrs = ["OsClockInterface.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "timers_os_clock",
    srcs = [
        "TimeConversion.cpp",
        "Timers_OsClock.cpp",
    ],
    hdrs = [
        "TimeConversion.hpp",
        "Timers_OsClock.hpp",
    ],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

alias(
    name = "time_conversion",
    actual = ":timers_os_clock",
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
)

cc_library(
    name = "cycle_time_validator",
    srcs = ["CycleTimeValidator.cpp"],
    hdrs = ["CycleTimeValidator.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [":os_clock_interface"],
)

cc_library(
    name = "cycle_timer",
    srcs = ["CycleTimer.cpp"],
    hdrs = ["CycleTimer.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        ":os_clock_interface",
        ":timers_os_clock",
    ],
)

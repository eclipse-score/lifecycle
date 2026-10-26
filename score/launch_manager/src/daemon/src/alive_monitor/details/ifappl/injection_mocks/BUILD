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
    name = "mock_data_structures",
    testonly = True,
    hdrs = ["DataStructures.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifappl/",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifappl/injection_mocks",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_monitor_if_daemon",
    testonly = True,
    hdrs = ["MonitorIfDaemon.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifappl/",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifappl/injection_mocks",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":mock_data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "@googletest//:gtest_main",
    ],
)

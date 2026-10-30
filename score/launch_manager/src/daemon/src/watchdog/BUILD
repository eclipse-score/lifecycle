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

# Interface for watchdog implementations. Public, so that integrators can provide their own implementation and
# select it with --@score_lifecycle//config:watchdog_impl=//my/pkg:my_watchdog
cc_library(
    name = "i_watchdog",
    hdrs = [
        "i_watchdog.hpp",
        "watchdog_factory.hpp",
    ],
    include_prefix = "score/mw/launch_manager/watchdog",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "mock_i_watchdog",
    testonly = True,
    hdrs = ["mock_i_watchdog.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":i_watchdog",
        "@googletest//:gtest",
    ],
)

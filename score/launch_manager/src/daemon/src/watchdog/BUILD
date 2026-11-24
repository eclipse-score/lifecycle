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
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "iwatchdog",
    hdrs = [
        "critical_error.hpp",
        "iwatchdog.hpp",
        "watchdog_factory.hpp",
    ],
    include_prefix = "score/mw/launch_manager/watchdog",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog",
    visibility = ["//score/launch_manager:__subpackages__"],
)

cc_library(
    name = "mock_iwatchdog",
    testonly = True,
    hdrs = ["mock_iwatchdog.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":iwatchdog",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "mock_critical_error",
    testonly = True,
    srcs = ["mock_critical_error.cpp"],
    hdrs = ["mock_critical_error.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":iwatchdog",
        "@googletest//:gtest",
    ],
)

lm_cc_test(
    name = "critical_error_UT",
    srcs = ["critical_error_UT.cpp"],
    deps = [
        ":iwatchdog",
        "@googletest//:gtest_main",
    ],
)

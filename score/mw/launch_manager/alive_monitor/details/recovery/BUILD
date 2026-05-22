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
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "notification",
    srcs = ["Notification.cpp"],
    hdrs = ["Notification.hpp"],
    visibility = ["//score/mw/launch_manager/alive_monitor:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/alive_monitor/details/logging:phm_logging",
        "//score/mw/launch_manager/recovery_client",
    ],
)

cc_test(
    name = "Notification_UT",
    srcs = ["Notification_UT.cpp"],
    deps = [
        ":notification",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:backend_stub_testutil",
    ],
)

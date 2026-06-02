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
    include_prefix = "score/mw/launch_manager/alive_monitor/details/recovery",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/recovery",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/alive_monitor/details/logging:phm_logging",
        "//score/launch_manager/daemon/src/recovery_client",
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

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

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "accessor_report_running_impl",
    testonly = True,
    hdrs = ["accessor_report_running_impl.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/lifecycle_client/src/details:report_running_impl",
        "@googletest//:gtest_main",
    ],
)

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
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "component_restart_files",
    hdrs = ["component_restart_files.hpp"],
    visibility = ["//tests/integration/component_restart:__subpackages__"],
)

cc_binary(
    name = "process_crashing_after_ready",
    srcs = ["process_crashing_after_ready.cpp"],
    visibility = ["//tests/integration/component_restart:__subpackages__"],
    deps = [
        ":component_restart_files",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "dependent_process",
    srcs = ["dependent_process.cpp"],
    visibility = ["//tests/integration/component_restart:__subpackages__"],
    deps = [
        ":component_restart_files",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

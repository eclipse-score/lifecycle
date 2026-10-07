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
    name = "mock_syscalls",
    hdrs = ["mock_syscalls.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_semaphore",
    testonly = True,
    srcs = ["mock_semaphore.cpp"],
    hdrs = ["mock_semaphore.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "@googletest//:gtest_main",
    ],
)

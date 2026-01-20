# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
load("@rules_rust//rust:defs.bzl", "rust_library")

cc_library(
    name = "lifecycle_client",
    srcs = [
        "src/lifecycleclient.cpp",
        "src/lifecycleclientimpl.cpp",
        "src/lifecycleclientimpl.hpp",
    ],
    hdrs = [
        "include/score/lcm/lifecycle_client.h",
    ],
    copts = select({
        "//config:x86_64-qnx": [],
        "//config:build_qnx8": [],
        "//conditions:default": ["-pthread"],
    }),
    includes = ["include"],
    linkopts = select({
        "//config:x86_64-qnx": [],
        "//config:build_qnx8": [],
        "//conditions:default": ["-lpthread"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//src/launch_manager_daemon/common",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "//src/launch_manager_daemon/common:log",
        "//src/launch_manager_daemon/common:osal",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

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

load("@score_baselibs//score/flatbuffers/bazel:codegen.bzl", "generate_cpp")

exports_files([
    "lm_flatcfg.fbs",
    "lm_flatcfg_generated.h",
])

filegroup(
    name = "new_lm_flatcfg_fbs",
    srcs = ["src/new_lm_flatcfg.fbs"],
    visibility = ["//visibility:public"],
)

generate_cpp(
    name = "new_lm_flatcfg_generated",
    output = "new_lm_flatcfg_generated.h",
    schema = "src/new_lm_flatcfg.fbs",
)

cc_library(
    name = "config",
    srcs = ["src/config.cpp"],
    hdrs = ["include/config.hpp"],
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
)

cc_library(
    name = "config_loader",
    hdrs = ["include/config_loader.hpp"],
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
    deps = [
        ":config",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "flatbuffer_config_loader",
    srcs = [
        "src/flatbuffer_config_loader.cpp",
        ":new_lm_flatcfg_generated",
    ],
    hdrs = ["include/flatbuffer_config_loader.hpp"],
    includes = [
        ".",
        "include",
        "src",
    ],
    visibility = ["//src:__subpackages__"],
    deps = [
        ":config_loader",
        "//src/launch_manager_daemon/common:log",
        "@flatbuffers",
        "@score_baselibs//score/flatbuffers:flatbufferutils",
    ],
)

cc_test(
    name = "flatbuffer_config_loader_UT",
    srcs = ["src/flatbuffer_config_loader_UT.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":flatbuffer_config_loader",
        "@flatbuffers",
        "@googletest//:gtest_main",
    ],
)

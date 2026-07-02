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
load("@score_baselibs//score/flatbuffers/bazel:codegen.bzl", "generate_cpp")

exports_files(
    [
        "lm_flatcfg.fbs",
        "launch_manager.schema.json",
    ],
    visibility = ["//visibility:public"],
)

exports_files(
    [
        "examples/example_conf.json",
    ] + glob([
        "default_values/**/*.json",
    ]),
    visibility = ["//:__subpackages__"],
)

generate_cpp(
    name = "fbs_config",
    output = "lm_flatcfg_generated.h",
    schema = "lm_flatcfg.fbs",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "config_schema",
    hdrs = [":fbs_config"],
    include_prefix = "score/mw/launch_manager/configuration",
    visibility = ["//score/launch_manager/src/daemon/src/configuration:__subpackages__"],
    deps = ["@score_baselibs//score/flatbuffers:flatbufferscpp"],
)

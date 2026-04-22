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
    "hm_flatcfg.fbs",
    "hmcore_flatcfg.fbs",
    ],
    visibility = ["//visibility:public"],
)

generate_cpp(
    name = "hm_config",
    output = "hm_flatcfg_generated.h",
    schema = "hm_flatcfg.fbs",
    visibility = ["//visibility:private"],
)

generate_cpp(
    name = "hmcore_config",
    output = "hmcore_flatcfg_generated.h",
    schema = "hmcore_flatcfg.fbs",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "config",
    hdrs = [
        ":hm_config",
        ":hmcore_config",
    ],
    strip_include_prefix = ".",
    visibility = ["//visibility:public"],
)

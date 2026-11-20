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
load("@rules_python//python:defs.bzl", "py_binary", "py_test")
load("@score_baselibs//score/flatbuffers/bazel:tools.bzl", "generate_json_schema", "serialize_buffer")
load("@score_lifecycle_pip//:requirements.bzl", "requirement")

exports_files(
    [
        "health_monitor_config.fbs",
        "health_monitor_config_schema.json",
    ],
    visibility = ["//visibility:public"],
)

# health_monitor_config.fbs is the single source of truth for the configuration.
# The JSON schema health_monitor_config_schema.json is generated from it and checked in;
# schema_test keeps the two in sync.
generate_json_schema(
    name = "health_monitor_config_postproc_schema",
    output = "health_monitor_config.postproc.schema.json",
    schema = "health_monitor_config.fbs",
    target_compatible_with = ["@platforms//os:linux"],
)

# Regenerates health_monitor_config_schema.json from health_monitor_config.fbs.
#   bazel run //score/health_monitor/configuration:generate_schema
py_binary(
    name = "generate_schema",
    srcs = ["generate_schema.py"],
    args = [
        "--in",
        "$(location :health_monitor_config_postproc_schema)",
        "--output",
        "$(rootpath health_monitor_config_schema.json)",
    ],
    data = [
        "health_monitor_config_schema.json",
        ":health_monitor_config_postproc_schema",
    ],
    target_compatible_with = ["@platforms//os:linux"],
)

py_test(
    name = "schema_test",
    srcs = [
        "generate_schema.py",
        "schema_test.py",
    ],
    args = [
        "--generated",
        "$(rootpath :health_monitor_config_postproc_schema)",
        "--schema",
        "$(rootpath health_monitor_config_schema.json)",
        "--example",
        "$(rootpath example/health_monitor_config.json)",
    ],
    data = [
        "example/health_monitor_config.json",
        "health_monitor_config_schema.json",
        ":health_monitor_config_postproc_schema",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [requirement("jsonschema")],
)

# Converting the example with flatc fails the build if it does not match the .fbs.
serialize_buffer(
    name = "health_monitor_config_example_bin",
    data = "example/health_monitor_config.json",
    output = "health_monitor_config_example.bin",
    schema = "health_monitor_config.fbs",
)

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
load("@score_baselibs//score/flatbuffers/bazel:tools.bzl", "generate_json_schema")
load("//:defs.bzl", "health_monitor_config")

exports_files(
    [
        "example/health_monitor_config.json",
        "health_monitor_config.fbs",
        "health_monitor_config_schema.json",
    ],
    visibility = ["//visibility:public"],
)

# health_monitor_config.fbs is the single source of truth for the configuration.
# The JSON schema health_monitor_config_schema.json is generated from it and checked in;
# //scripts/config_mapping/tests/health_monitor:health_monitor_tests keeps the two in sync.
# Regenerate it with:
#   bazel run //scripts/config_mapping:generate_health_monitor_schema
generate_json_schema(
    name = "health_monitor_config_postproc_schema",
    output = "health_monitor_config.postproc.schema.json",
    schema = "health_monitor_config.fbs",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//scripts/config_mapping:__subpackages__"],
)

# Building the example validates it against the JSON schema and converts it with flatc.
# Either step fails the build if the example does not match.
health_monitor_config(
    name = "health_monitor_config_example",
    config = "example/health_monitor_config.json",
)

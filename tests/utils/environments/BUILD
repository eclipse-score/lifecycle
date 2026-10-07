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

load("//config:mw_com_config.bzl", "mw_com_config")

exports_files(
    ["ecu_logging_config.json"],
    visibility = ["//tests/utils/environments:__subpackages__"],
)

mw_com_config(
    name = "mw_com_config_gen",
    out = "mw_com_config.json",
    profile = "integration",
    visibility = [
        "//examples/demo_verification:__subpackages__",
        "//tests/integration:__subpackages__",
    ],
)

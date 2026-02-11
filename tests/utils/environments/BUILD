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

filegroup(
    name = "target_config",
    srcs = ["target_config.json"],
    visibility = ["//tests/it:__subpackages__"],
)

# Empty target for native builds
filegroup(
    name = "empty_target",
    srcs = [],
    visibility = [
        "//tests:__subpackages__",
    ],
)

alias(
    name = "test_environment",
    actual = select({
        "@platforms//os:qnx": "//tests/utils/environments/qnx8_qemu:init_build",
        "//conditions:default": ":empty_target",
    }),
    visibility = [
        "//tests:__subpackages__",
    ],
)

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

load("@score_toolchains_qnx//rules/fs:ifs.bzl", "qnx_ifs")

qnx_ifs(
    name = "init_build",
    srcs = [
        "system.build",
        "//tests/utils/environments/qnx8_qemu/configs",
    ],
    build_file = "init.build",
    tags = ["manual"],  # Excluded from wildcard builds
    visibility = [
        "//tests:__subpackages__",
    ],
)

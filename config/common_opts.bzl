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
SANITIZER_CXXOPTS = select({
    "//config:thread_sanitizer_enabled": ["-fsanitize=thread", "-O0", "-g"],
    "//config:ub_sanitizer_enabled": ["-fsanitize=undefined", "-O0", "-g"],
    "//config:address_sanitizer_enabled": [
        "-fsanitize=address",
        "-fno-omit-frame-pointer",
        "-O1",
        "-g",
    ],
    "//conditions:default": [],
})

SANITIZER_LINKOPTS = select({
    "//config:thread_sanitizer_enabled": ["-fsanitize=thread"],
    "//config:ub_sanitizer_enabled": ["-fsanitize-link-c++-runtime", "-fsanitize=undefined"],
    "//config:address_sanitizer_enabled": ["-fsanitize=address"],
    "//conditions:default": [],
})

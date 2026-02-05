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
cc_binary(
    name = "cpp_lifecycle_app",
    srcs = [
        "main.cpp",
    ],
    copts = select({
        "//config:build_qnx8": [],
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-pthread"],
    }),
    linkopts = select({
        "//config:build_qnx8": [],
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-lpthread"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//src/lifecycle_client_lib:lifecycle_client_lib",
        
    ],
)

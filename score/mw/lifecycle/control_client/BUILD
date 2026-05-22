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

cc_library(
    name = "control_client",
    srcs = [
        "control_client.cpp",
    ],
    hdrs = [
        "control_client.h",
    ],
    include_prefix = "score/mw/lifecycle",
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/launch_manager/common:identifier_hash",
        "//score/mw/lifecycle/control_client/details:control_client_impl",
    ],
)

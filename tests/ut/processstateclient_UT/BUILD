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
cc_test(
    name = "processstateclient_UT",
    srcs = [
        "processstateclient_UT.cpp",
        "//src/launch_manager_daemon/process_state_client_lib:src/processstatenotifier.cpp",
        "//src/launch_manager_daemon/process_state_client_lib:src/processstatereceiver.cpp",
    ],
    visibility = ["//tests:__subpackages__"],
    deps = [
        "//src/launch_manager_daemon/process_state_client_lib:headers",
        "@googletest//:gtest_main",
    ],
)

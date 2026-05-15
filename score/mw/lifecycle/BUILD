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

package(default_visibility = ["//visibility:public"])

alias(
    name = "lifecycle_cc",
    actual = "//src/lifecycle_client_lib:lifecycle_client_lib",
)

alias(
    name = "lifecycle_rust",
    actual = "//src/launch_manager_daemon/lifecycle_client_lib/rust_bindings:lifecycle_client_rs",
)

alias(
    name = "control_cc",
    actual = "//src/control_client_lib:control_client_lib",
)

alias(
    name = "alive_cc",
    actual = "//src/launch_manager_daemon/health_monitor_lib:hm_shared_lib",
)

alias(
    name = "alive_rust",
    actual = "//src/launch_manager_daemon/health_monitor_lib/rust_bindings:monitor_rs",
)

alias(
    name = "lifecycle",
    actual = ":lifecycle_cc",
)

alias(
    name = "control",
    actual = ":control_cc",
)

alias(
    name = "alive",
    actual = ":alive_cc",
)

alias(
    name = "launch_manager",
    actual = "//src/launch_manager_daemon:launch_manager",
)

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
config_setting(
    name = "x86_64-linux",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
)

config_setting(
    name = "arm64-linux",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:arm64",
    ],
)

config_setting(
    name = "x86_64-qnx",
    constraint_values = [
        "@platforms//os:qnx",
        "@platforms//cpu:x86_64",
    ],
)

config_setting(
    name = "arm64-qnx",
    constraint_values = [
        "@platforms//os:qnx",
        "@platforms//cpu:arm64",
    ],
)

config_setting(
    name = "host",
    define_values = {
        "config": "host",
    },
)

config_setting(
    name = "thread_sanitizer_enabled",
    define_values = {"sanitize": "thread"},
)

config_setting(
    name = "address_sanitizer_enabled",
    define_values = {"sanitize": "address"},
)

config_setting(
    name = "ub_sanitizer_enabled",
    define_values = {"sanitize": "undefined"},
)

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
load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")

oci_image(
    name = "image",
    base = "@debian-test-runtime",
)

oci_load(
    name = "x86_64-linux",
    image = ":image",
    repo_tags = ["score_itf_examples:latest"],
    visibility = [
        "//tests:__subpackages__",
    ],
)

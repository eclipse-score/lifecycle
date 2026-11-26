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

"""DWYU aspect: checks that every Bazel target depends on what it includes."""

load("@depend_on_what_you_use//dwyu/cc:defs.bzl", "dwyu_cc_aspect_factory")

dwyu = dwyu_cc_aspect_factory(
    # assertion_handler is never included, it only has to be linked (alwayslink).
    ignored_unused_deps = [Label("//score/launch_manager/src/daemon/src/common:assertion_handler")],
    # Only analyze targets of this repository, not external dependencies.
    skip_external_targets = True,
    target_mapping = Label("//quality/dwyu:target_mapping"),
)

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

load("@depend_on_what_you_use//dwyu/cc:defs.bzl", "MAP_DIRECT_DEPS", "MAP_EXPLICIT_DEPS", "dwyu_make_cc_info_mapping")
load(":canonical.bzl", "canonical")

# Some public targets of external modules only forward headers from targets that are not visible to us.
# Tell DWYU that the public target provides those headers.
dwyu_make_cc_info_mapping(
    name = "target_mapping",
    mapping = {
        # score/filesystem/path.h is in :path, which :filesystem depends on.
        "@score_baselibs//score/filesystem": {
            MAP_EXPLICIT_DEPS: [canonical("@score_baselibs//score/filesystem:path")],
        },
        # score/os/utils/signal.h and signal_impl.h are in :signal_headers.
        "@score_baselibs//score/os/utils:signal": {
            MAP_DIRECT_DEPS: [canonical("@score_baselibs//score/os/utils:signal_headers")],
        },
        # score/os/utils/thread.h is in :thread_headers.
        "@score_baselibs//score/os/utils:thread": {
            MAP_DIRECT_DEPS: [canonical("@score_baselibs//score/os/utils:thread_headers")],
        },
        # score/mw/com has no headers itself, it bundles :types, :runtime and the other mw::com targets.
        "@score_communication//score/mw/com": {
            MAP_DIRECT_DEPS: "all",
        },
    },
    visibility = ["//visibility:public"],
)

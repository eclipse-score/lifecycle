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

# Local, uncompiled CodeQL query pack providing file/function size and
# complexity metrics (ported from vsps_quality_packages//tools/codeql/code-complexity).
# Resolved by codeql_lint.py via --additional-packs=third_party/codeql (relative
# to the repo root), not via Bazel runfiles, since it is analyzed in place as
# part of the traced-build source tree rather than fetched as an external
# dependency.
filegroup(
    name = "pack",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

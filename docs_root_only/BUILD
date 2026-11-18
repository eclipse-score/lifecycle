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

load("@score_docs_as_code//:docs.bzl", "docs_bundle")

# Docs mounted by //:BUILD only in lifecycle's own doc build, not in ref_int.
# Kept outside docs/, because Sphinx reads docs/ directly and would always pick them up.
docs_bundle(
    name = "docs",
    srcs = ["index.rst"],  # Needs the extension docs/feat_req_verification_table.py
    visibility = ["//:__pkg__"],
)

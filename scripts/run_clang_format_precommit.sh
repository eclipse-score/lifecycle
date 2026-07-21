#!/usr/bin/env bash
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
set -e

clang_format_bin=$(bazel cquery @llvm_toolchain//:clang-format --output files 2>/dev/null)
if [ -f "$clang_format_bin" ]; then
    "$clang_format_bin" -i "$@"
else
    bazel build @llvm_toolchain//:clang-format
    "$clang_format_bin" -i "$@"
fi

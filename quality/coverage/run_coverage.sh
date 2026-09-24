#!/bin/bash

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

set -euo pipefail

# Switch back to BUILD_WORKSPACE_DIRECTORY when invoked via bazel
if [[ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
  workspace_root="$BUILD_WORKSPACE_DIRECTORY"
else
  workspace_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
fi
cd -- "$workspace_root"

# 1. Collect coverage
# Note: Targets with "no-coverage" tag are skipped
bazel coverage --config=llvm_cov //score/... --lockfile_mode=error --build_tests_only

# 2. Generate the HTML report
bazel run @score_coverage//:generate_coverage_html -- \
  --yaml quality/coverage/coverage_justifications.yaml \
  --archive-dir coverage_artifacts

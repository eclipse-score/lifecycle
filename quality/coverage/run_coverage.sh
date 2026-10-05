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

usage() {
  echo "Usage: bazel run //quality/coverage:run_coverage -- [--platform=linux|qnx]" >&2
}

platform="linux"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --platform=*) platform="${1#*=}" ;;
    --platform)
      [[ $# -ge 2 ]] || { usage; exit 1; }
      platform="$2"
      shift
      ;;
    -h | --help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 1 ;;
  esac
  shift
done

case "$platform" in
  linux)
    # 1. Collect coverage
    # Note: Targets with "no-coverage" tag are skipped
    bazel coverage --config=llvm_cov //score/... --lockfile_mode=error --build_tests_only

    # 2. Generate the HTML report
    bazel run @score_coverage//:generate_coverage_html -- \
      --yaml quality/coverage/coverage_justifications.yaml \
      --archive-dir coverage_artifacts
    ;;
  qnx)
    # 1. Collect coverage
    bazel coverage --config=gcov --config=unit-tests-x86_64-qnx //score/launch_manager/... --build_tests_only

    # 2. Generate the HTML report
    bazel run @score_coverage//:generate_coverage_html -- --platform qnx \
      --yaml quality/coverage/coverage_justifications.yaml \
      --archive-dir coverage_qnx_artifacts
    ;;
  *)
    echo "Unsupported platform: $platform" >&2
    usage
    exit 1
    ;;
esac

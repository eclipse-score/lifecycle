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
echo "Pass the BAZEL targets as arguments, e.g.: //src/... "

# Exit immediately if any command fails
set -e


# Check if SCORE_QNX_USER and SCORE_QNX_PASSWORD are set
if [[ -z "${SCORE_QNX_USER}" || -z "${SCORE_QNX_PASSWORD}" ]]; then
  # If either is missing, check for .netrc
  if [[ ! -f "$HOME/.netrc" ]]; then
    echo "Error: Either SCORE_QNX_USER/SCORE_QNX_PASSWORD must be set, or $HOME/.netrc must exist."
    exit 1
  fi
fi

echo "Note: If it fails with 'Error downloading [https://www.qnx.com/download/download/79858/installation.tgz]' access this link from webbrowser and accept the license agreement."

# Use default argument //src/... if none provided
if [ $# -eq 0 ]; then
  set -- //src/...
fi

bazel build --config=lc-x86_64-qnx --credential_helper=*.qnx.com=%workspace%/scripts/internal/qnx_creds.py "$@"

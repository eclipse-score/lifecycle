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

"""Unified entrypoint for lifecycle Bazel macros & rules."""

# --- Launch Manager Configuration ---
load("//scripts/config_mapping:config.bzl", _launch_manager_config = "launch_manager_config")

launch_manager_config = _launch_manager_config

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

# Default installation prefix for test binaries on target systems.
# Change this value to relocate all test binaries to a different base path.
# Note that if any directory other than /tmp is used, you must also use the
# --sandbox_writable_path=/your/test/path argument
SCORE_TEST_INSTALL_PREFIX = "/tmp"

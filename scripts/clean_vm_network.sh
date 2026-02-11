#!/bin/sh
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

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root (use sudo)" >&2
    exit 1
fi

BRIDGE_DEVICE="br0"
TAP_DEVICE="tap0"

ip link set "$TAP_DEVICE" nomaster
ip link set "$TAP_DEVICE" down
ip link delete "$TAP_DEVICE"
ip link set "$BRIDGE_DEVICE" down
ip link delete "$BRIDGE_DEVICE"

echo "Network cleaned up: bridge $BRIDGE_DEVICE with $TAP_DEVICE"

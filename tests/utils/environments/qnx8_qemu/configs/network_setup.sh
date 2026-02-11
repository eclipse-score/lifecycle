#!/bin/sh

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


# *******************************************************************************
# Network Configuration Script
# Configures networking interfaces and settings for QNX system
# *******************************************************************************

echo "---> Starting Networking"
io-sock -m phy -m pci -d vtnet_pci    # Start network stack with PHY and PCI modules, load VirtIO network driver
waitfor /dev/socket                    # Wait for socket device to become available before proceeding

echo "---> Configuring network interface"
# Bring up the interface and configure with bridge-accessible IP for direct ping
if_up -p vtnet0                        # Bring up the vtnet0 network interface in promiscuous mode
ifconfig vtnet0 192.168.100.10 netmask 255.255.255.0  # Configure IP address and subnet mask for vtnet0

# Configure system network settings
sysctl -w net.inet.icmp.bmcastecho=1 > /dev/null        # Enable ICMP broadcast echo (responds to broadcast pings)
sysctl -w qnx.sec.droproot=33:33 > /dev/null            # Set user/group ID (33:33) for dropping root privileges

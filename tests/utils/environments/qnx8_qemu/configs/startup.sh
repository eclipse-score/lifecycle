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
# System Startup Script
# Executed during system initialization to start essential services
# *******************************************************************************
echo "---> Starting slogger2"
slogger2 -s 4096                       # Start system logger with 4KB buffer size for log messages
waitfor /dev/slog                       # Wait for system log device to become available

echo "---> Starting PCI Services"
pci-server --config=/proc/boot/pci_server.cfg  # Start PCI server with configuration file
waitfor /dev/pci                        # Wait for PCI device manager to initialize

echo "---> Starting Pipe"
pipe                                    # Start named pipe resource manager for IPC
waitfor /dev/pipe                       # Wait for pipe device to become available

echo "---> Starting Random"
random                                  # Start random number generator device
waitfor /dev/random                     # Wait for random device to become available

echo "---> Starting fsevmgr"
fsevmgr                                 # Start file system event manager for file notifications
waitfor /dev/fsnotify                   # Wait for filesystem notification device

echo "---> Starting devb-ram"
devb-ram ram capacity=1 blk ramdisk=10m,cache=512k,vnode=256 2>/dev/null  # Create 10MB RAM disk with 512KB cache
waitfor /dev/ram0                       # Wait for RAM disk device to be ready

echo "---> mounting ram disk"
mkqnx6fs -q /dev/ram0                   # Create QNX6 filesystem on RAM disk (quiet mode)
waitfor /dev/ram0                       # Wait for filesystem creation to complete
mount -t qnx6 /dev/ram0 /tmp_ram        # Mount RAM disk as QNX6 filesystem at /tmp_ram

echo "---> Starting mqueue"
mqueue                                  # Start POSIX message queue resource manager
waitfor /dev/mqueue                     # Wait for message queue device to be available

echo "---> Starting devc-pty"
devc-pty -n 32                          # Start pseudo-terminal driver with 32 PTY pairs

echo "---> Starting devb-eide"
devb-eide cam user=20:20 blk cache=64M,auto=partition,vnode=2000,ncache=2000,commit=low  # Start IDE/SATA block driver
waitfor /dev/hd0                        # Wait for first hard disk to be detected

echo "---> Configuring network"
/etc/network_setup.sh # fixed IP setup --> commented out as dhcp is now in use

echo "---> Setting hostname"
if [ -f /etc/hostname ]; then           # Check if hostname file exists
    HOSTNAME=$(cat /etc/hostname)       # Read hostname from file
    hostname "$HOSTNAME"                # Set system hostname
    echo "Hostname set to: $HOSTNAME"
else
    hostname qnx-score                  # Set default hostname if no file exists
    echo "Qnx_S-core" > /tmp/hostname   # Create temporary hostname file
    echo "Default hostname set to: Qnx_S-core"
fi

echo "---> adding /tmp_discovery folder"
mkdir -p /tmp_ram/tmp_discovery
ln -sP  /tmp_ram/tmp_discovery /tmp_discovery

/proc/boot/sshd -f /var/ssh/sshd_config -E /tmp/sshd.log # Start SSH daemon with specified configuration file

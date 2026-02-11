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

from tests.utils.fixtures.target import target
# from tests.utils.fixtures.ssh import ssh
from tests.utils.fixtures import control_interface

import logging
logger = logging.getLogger(__name__)

def test_one(control_interface):
    ret, stdout, stderr = control_interface.exec_command_blocking("ls")
    logger.error(stdout)

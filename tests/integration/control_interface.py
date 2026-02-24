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
from typing import Tuple
from pathlib import Path
from abc import ABC, abstractmethod


class ControlInterface(ABC):
    """Platform independent interface to execute commands on the target"""

    @abstractmethod
    def exec_command_blocking(
        *args: str, timeout=1, **env: str
    ) -> Tuple[int, str, str]:
        """Execute a command on the target

        Args:
            *args (str): Command to run with arguments
            timeout (int): Time in seconds to exit after, returning status -1
            **env (str): Environment vars to set

        Returns:
            (int, str, str): exit_status, stdout, stderr
        """
        raise NotImplementedError()

    @abstractmethod
    def run_until_file_deployed(
        *args,
        timeout=1,
        file_path=Path("tests/integration/test_end"),
        poll_interval=0.05,
        **env,
    ) -> Tuple[int, str, str]:
        """Launch a process and terminate it once a given file has been deployed

        Args:

            *args (str): Command to run with arguments
            timeout (int): Time in seconds to exit after, returning status -1
            file_path (Path): File to wait for
            poll_interval (float): How often, in seconds, to check if we should terminate the process
            **env (str): Environment vars to set

        Returns:
            (int, str, str): exit_status, stdout, stderr
        """
        raise NotImplementedError()

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
from typing import Union, Optional, List, Dict


class ControlInterface(ABC):
    """Platform independent interface to execute commands on the target"""

    _TIMEOUT_CODE = -1
    _FILE_FOUND_CODE = 0

    @abstractmethod
    def exec_command_blocking(
        self,
        args: Union[str, List[str]],
        cwd: Optional[Path] = None,
        timeout: int = 1,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        """Execute a command on the target

        Args:
            args: Arguments for the command.
            cwd: Working directory to execute the command in.
            timeout: Timeout in seconds to run the command.
            env: Dictionary of environmental variables to execute the
                 command under.

        Returns:
            return code, stdout, stderr
        """
        raise NotImplementedError()

    @abstractmethod
    def run_until_file_deployed(
        self,
        args: Union[str, List[str]],
        file_path: Path,
        cwd: Optional[Path] = None,
        timeout: int = 1,
        poll_interval: float = 0.05,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        """Launch a process and terminate it once a given file has been deployed

        Args:
            args: Arguments for the command.
            file_path: The path of the file to check.
            cwd: Working directory to execute the command in.
            timeout: Timeout in seconds to run the command.
            poll_interval: Interval in seconds to check the files existance.
            env: Dictionary of environmental variables to execute the
                 command under.

        Returns:
            return code, stdout, stderr
        """

        raise NotImplementedError()

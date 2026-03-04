from tests.utils.fixtures.target.fixture import target
from tests.utils.fixtures.control_interface.control_interface import ControlInterface
from itf.core.com.ssh import execute_command, Ssh
from typing import Optional, Tuple, Union, List, Dict
from pathlib import Path
import pytest
import logging
import time


logger = logging.getLogger(__name__)


class SshInterface(ControlInterface):
    def __init__(self, ssh: Ssh):
        self.__ssh = ssh

    def exec_command_blocking(
        self,
        args: Union[str, List[str]],
        cwd: Optional[Path] = None,
        timeout=1,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        if isinstance(args, list):
            args = " ".join(args)

        if cwd:
            args = f"cd {str(cwd)} && " + args

        if not env:
            env = {}

        env.update({"PATH": "/proc/boot"})

        for key, value in env.items():
            args = f"{key}={value} " + args

        logger.debug(f"Running command '{args}'")

        _, stdout, stderr = self.__ssh.exec_command(
            args, environment=env, timeout=timeout
        )

        ret_code = stdout.channel.recv_exit_status()

        return ret_code, stdout.read().decode("utf-8"), stderr.read().decode("utf-8")

    def run_until_file_deployed(
        self,
        args: Union[str, List[str]],
        file_path: Path,
        cwd: Optional[Path] = None,
        timeout=1,
        poll_interval=0.05,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        if isinstance(args, list):
            args = " ".join(str(arg) for arg in args)

        if not env:
            env = {}

        if cwd:
            args = f"cd {str(cwd)} && " + args

        file_path_str = str(file_path)

        _, stdout, stderr = self.__ssh.exec_command(args, environment=env)
        channel = stdout.channel

        start_time = time.time()
        file_found = False

        try:
            while True:
                # check if timeout exceeded
                if (time.time() - start_time) > timeout:
                    # timeout - kill the process
                    stdout.channel.close()
                    return (
                        self._TIMEOUT_CODE,
                        stdout.read().decode("utf-8", errors="replace"),
                        stderr.read().decode("utf-8", errors="replace"),
                    )

                # check if process has ended
                if channel.exit_status_ready():
                    break

                # check if file exists on remote system
                check_cmd = (
                    f"test -f {file_path_str} && echo 'EXISTS' || echo 'NOT_EXISTS'"
                )
                _, check_stdout, _ = self.__ssh.exec_command(check_cmd)
                result = check_stdout.read().decode("utf-8", errors="replace").strip()

                if result == "EXISTS":
                    file_found = True
                    break
                else:
                    logger.debug("file not found")

                logger.debug("waiting")
                time.sleep(poll_interval)

            # if file was found, kill the process
            if file_found:
                stdout.channel.close()
                return (
                    self._FILE_FOUND_CODE,
                    stdout.read().decode("utf-8", errors="replace"),
                    stderr.read().decode("utf-8", errors="replace"),
                )

            # process ended naturally
            exit_code = channel.recv_exit_status()
            output = stdout.read().decode("utf-8", errors="replace")
            error = stderr.read().decode("utf-8", errors="replace")

            return (exit_code, output, error)

        except Exception as e:
            # ensure we close the channel on any exception
            try:
                stdout.channel.close()
            except:
                pass

            try:
                output = stdout.read().decode("utf-8", errors="replace")
                error = stderr.read().decode("utf-8", errors="replace")
            except:
                output = ""
                error = str(e)

            return (-1, output, error)

    @property
    def ssh(self):
        return self.__ssh

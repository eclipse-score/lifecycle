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
import logging
import shutil
import signal
import subprocess
import threading
import time
from pathlib import Path
from typing import List

import pytest

from score.itf.core.process.async_process import AsyncProcess
from score.itf.core.target import Target
from score.itf.plugins.core import determine_target_scope

logger = logging.getLogger(__name__)


def pytest_addoption(parser):
    parser.addoption(
        "--local-dir",
        action="store",
        required=True,
        help="Base directory on the local host to use as the target root.",
    )
    parser.addoption(
        "--no-local-cleanup",
        action="store_false",
        dest="local_cleanup",
        default=True,
        help="Skip cleanup of the local target directory after each test.",
    )


class LocalAsyncProcess(AsyncProcess):
    def __init__(
        self,
        process: subprocess.Popen,
        output_lines: List[str],
        output_thread: threading.Thread,
    ):
        self._process = process
        self._output_lines = output_lines
        self._output_thread = output_thread

    def pid(self) -> int:
        return self._process.pid

    def is_running(self) -> bool:
        return self._process.poll() is None

    def get_exit_code(self) -> int:
        return self._process.poll()

    def wait(self, timeout_s: float = 15) -> int:
        try:
            self._process.wait(timeout=timeout_s)
        except subprocess.TimeoutExpired:
            raise RuntimeError(
                f"Waiting for process with PID [{self._process.pid}] timed out after {timeout_s}s"
            )
        self._output_thread.join()
        return self._process.returncode

    def stop(self) -> int:
        if self.is_running():
            self._process.send_signal(signal.SIGTERM)
            for _ in range(5):
                time.sleep(1)
                if not self.is_running():
                    break
            if self.is_running():
                logger.error(
                    f"Process [{self._process.pid}] did not terminate, sending SIGKILL"
                )
                self._process.send_signal(signal.SIGKILL)
                self._process.wait()
        self._output_thread.join()
        return self._process.returncode

    def get_output(self) -> str:
        return "\n".join(self._output_lines) + ("\n" if self._output_lines else "")


class LocalTarget(Target):
    def __init__(self, base_dir: str):
        super().__init__()
        self._base_dir = Path(base_dir)
        self._base_dir.mkdir(parents=True, exist_ok=True)

    def execute(self, command: str):
        result = subprocess.run(
            command,
            shell=True,
            capture_output=True,
            cwd=self._base_dir,
        )
        return result.returncode, result.stdout + result.stderr

    def upload(self, local_path: str, remote_path: str) -> None:
        dest = Path(remote_path)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(local_path, dest)

    def download(self, remote_path: str, local_path: str) -> None:
        Path(local_path).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(remote_path, local_path)

    def execute_async(
        self, binary_path: str, args=None, cwd: str = "/", **kwargs
    ) -> LocalAsyncProcess:
        cmd = ["fakeroot", "--", binary_path] + (args or [])
        cmd_logger = logging.getLogger(Path(binary_path).name)
        output_lines: List[str] = []

        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=cwd,
        )

        def _async_log(proc: subprocess.Popen):
            for raw in proc.stdout:
                line = raw.decode(errors="replace").rstrip()
                if line:
                    cmd_logger.info(line)
                    output_lines.append(line)

        output_thread = threading.Thread(
            target=_async_log, args=(process,), daemon=True
        )
        output_thread.start()

        return LocalAsyncProcess(process, output_lines, output_thread)

    def restart(self) -> None:
        # No-op: the local host does not restart between tests.
        pass


@pytest.fixture(scope=determine_target_scope)
def target_init(request):
    local_dir = request.config.getoption("--local-dir")
    do_cleanup = request.config.getoption("local_cleanup")
    yield LocalTarget(local_dir)
    if do_cleanup:
        shutil.rmtree(local_dir, ignore_errors=True)

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
import signal
import subprocess
import threading
import time
from typing import List, Optional, Tuple, Dict, Union
from pathlib import Path
import os
from .control_interface import ControlInterface
import logging


logger = logging.getLogger(__name__)


class LinuxControl(ControlInterface):
    def exec_command_blocking(
        self,
        args: Union[str, List[str]],
        cwd: Optional[Path] = None,
        timeout=1,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        if not env:
            env = {}

        try:
            res = subprocess.run(
                args, env=env, cwd=cwd, capture_output=True, text=True, timeout=timeout
            )
            return res.returncode, "".join(res.stdout), "".join(res.stderr)

        except subprocess.TimeoutExpired as ex:
            if not ex.stderr:
                stderr = ""
            else:
                stderr = ex.stderr.decode("utf-8")
            return self._TIMEOUT_CODE, str(ex.output.decode("utf-8")), stderr

    @staticmethod
    def _reader(stream, sink: List[str]):
        """Read text lines from a stream until EOF and append to sink."""
        try:
            for line in iter(stream.readline, ""):
                if not line:
                    break
                sink.append(line)
        finally:
            try:
                stream.close()
            except Exception:
                pass

    def _terminate_process_group(
        self, proc: subprocess.Popen, sigterm_timeout_seconds: float
    ):
        """Terminate all processes in a processgroup. Graceful termination is
        attempted before SIGKILL is sent"""
        if proc.poll() is not None:
            return  # already exited

        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except Exception:
            proc.terminate()

        deadline = time.time() + sigterm_timeout_seconds
        while time.time() < deadline:
            if proc.poll() is not None:
                return
            time.sleep(0.05)

        # Force kill
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except Exception:
            proc.kill()

    def run_until_file_deployed(
        self,
        args: Union[str, List[str]],
        file_path: Path,
        cwd: Optional[Path] = None,
        timeout=1,
        poll_interval=0.05,
        env: Optional[Dict[str, str]] = None,
    ) -> Tuple[int, str, str]:
        if not env:
            env = {}

        if isinstance(args, str):
            args = args.split(" ")

        proc = subprocess.Popen(
            ["/usr/bin/fakeroot"] + args,
            env=env,
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # start reader threads to capture stdout/stderr without blocking
        stdout_lines: List[str] = []
        stderr_lines: List[str] = []
        t_out = threading.Thread(
            target=LinuxControl._reader, args=(proc.stdout, stdout_lines), daemon=True
        )
        t_err = threading.Thread(
            target=LinuxControl._reader, args=(proc.stderr, stderr_lines), daemon=True
        )
        t_out.start()
        t_err.start()

        start = time.time()
        deadline = start + timeout

        exit_code: Optional[int] = None

        try:
            while True:
                rc = proc.poll()
                if rc is not None:  # exited already
                    exit_code = rc
                    break

                now = time.time()
                if file_path.exists():
                    exit_code = self._FILE_FOUND_CODE
                    self._terminate_process_group(proc, timeout)
                    os.remove(file_path)
                    break

                if now >= deadline:
                    exit_code = self._TIMEOUT_CODE
                    self._terminate_process_group(proc, timeout)
                    break

                time.sleep(poll_interval)
        except KeyboardInterrupt:
            self._terminate_process_group(proc, timeout)

        if exit_code is None:
            exit_code = -1

        # ensure readers finish
        t_out.join(timeout=2.0)
        t_err.join(timeout=2.0)

        return exit_code, "".join(stdout_lines), "".join(stderr_lines)

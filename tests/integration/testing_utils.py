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
import shutil
import threading
import time
from typing import List, Optional, Tuple, Literal
from pathlib import Path
import os
from tests.integration.control_interface import ControlInterface

_TIMEOUT_CODE = -1


class LinuxControl(ControlInterface):
    def exec_command_blocking(*args, timeout=1, **env) -> Tuple[int, str, str]:
        try:
            res = subprocess.run(
                args, env=env, capture_output=True, text=True, timeout=timeout
            )
            return res.returncode, "".join(res.stdout), "".join(res.stderr)
        except subprocess.TimeoutExpired as ex:
            return _TIMEOUT_CODE, ex.output.decode("utf-8"), ex.stderr

    def _reader(stream, sink: List[str]):
        """Read text lines from a stream until EOF and append to sink."""
        try:
            for line in stream:
                if not line:
                    break
                sink.append(line)
        finally:
            try:
                stream.close()
            except Exception:
                pass

    def _terminate_process_group(
        proc: subprocess.Popen, sigterm_timeout_seconds: float
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
        *args,
        timeout=1,
        file_path=Path("tests/integration/test_end"),
        poll_interval=0.05,
        **env,
    ) -> Tuple[int, str, str]:
        proc = subprocess.Popen(
            ("/usr/bin/fakeroot", "/usr/bin/fakechroot", "-s", "chroot", ".", *args),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=1,
            text=True,
            preexec_fn=os.setsid,  # start the process in its own process group so we can signal the whole group
        )

        # Start reader threads to capture stdout/stderr without blocking
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
                if rc is not None:  # Exited already
                    exit_code = rc
                    break

                now = time.time()

                if file_path.exists():
                    exit_code = 0
                    LinuxControl._terminate_process_group(proc, timeout)
                    os.remove(file_path)
                    break

                if now >= deadline:
                    exit_code = _TIMEOUT_CODE
                    LinuxControl._terminate_process_group(proc, timeout)
                    break

                time.sleep(poll_interval)
        except KeyboardInterrupt:
            LinuxControl._terminate_process_group(proc, timeout)

        # Ensure readers finish
        t_out.join(timeout=2.0)
        t_err.join(timeout=2.0)

        return exit_code, "".join(stdout_lines), "".join(stderr_lines)


def get_common_interface() -> ControlInterface:
    """Get a platform independent façade to execute commands on the target"""
    match get_platform():
        case "linux":
            return LinuxControl
        case "qemu":
            raise NotImplementedError("QEMU façade is not yet implemented")
        case _:
            raise KeyError("Platform not recognised")


def get_platform() -> Literal["linux", "qemu"]:
    return "linux"


def get_bazel_out_dir() -> Path:
    """Files written to this location are accessible from `bazel-out` when
    `--remote_download_outputs=all`
    """
    return Path(os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR"))


def check_for_failures(path: Path, expected_count: int):
    """Check expected_count xml files for failures, raising an exception if
    a failure is found or a different number of xml files are found.
    """
    failing_files = []
    checked_files = []
    for file in path.iterdir():
        if file.suffix == ".xml":
            gtest_xml = open(file).read()
            query = 'failures="'
            failure_number = gtest_xml[gtest_xml.find(query) + len(query)]
            if failure_number != "0":
                failing_files.append(file.name)
            checked_files.append(file.name)
            shutil.copy(file, get_bazel_out_dir())
    if len(failing_files) > 0:
        raise RuntimeError(
            f"Failures found in the following files:\n {'\n'.join(failing_files)}"
        )
    if len(checked_files) != expected_count:
        raise RuntimeError(
            f"Expected to find {expected_count} xml files, instead found {len(checked_files)}:\n{'\n'.join(checked_files)}"
        )


def format_logs(exit_code: int, stdout: str, stderr: str) -> str:
    """Human-readable format for exit code, stdout and stderr"""
    extra_info = " (timeout)" if exit_code == _TIMEOUT_CODE else ""
    return f"stdout:\n{stdout}\n\nstderr:\n{stderr}\n\nExit status = {exit_code}{extra_info}"

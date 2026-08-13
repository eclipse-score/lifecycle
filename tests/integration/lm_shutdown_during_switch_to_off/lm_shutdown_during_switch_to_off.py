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
import time

from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties

logger = logging.getLogger(__name__)


def _wait_for_file(target, file_path, proc, timeout_s):
    """Block until `file_path` exists on the target, or raise on timeout / early
    exit of the launch manager process."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if not proc.is_running():
            raise RuntimeError(
                f"Launch manager exited (code {proc.get_exit_code()}) before "
                f"'{file_path}' appeared. Output:\n{proc.get_output()}"
            )
        exit_code, _ = target.execute(f"test -f {file_path}")
        if exit_code == 0:
            return
        time.sleep(0.05)
    raise TimeoutError(f"'{file_path}' did not appear within {timeout_s}s")


def _pids_by_comm(target, name):
    """Return the PIDs whose process name matches `name`.

    The test sandbox provides neither ``pgrep`` nor ``ps``, so processes are
    located by scanning ``/proc/<pid>/comm`` using only shell builtins and
    ``cat``. Linux truncates the process name (``comm``) to 15 characters, so
    the target name is truncated the same way before comparing.
    """
    truncated = name[:15]
    scan = (
        "for p in /proc/[0-9]*/comm; do "
        'c=$(cat "$p" 2>/dev/null) || continue; '
        f'if [ "$c" = "{truncated}" ]; then '
        "q=${p#/proc/}; echo ${q%/comm}; fi; "
        "done"
    )
    exit_code, stdout = target.execute(scan)
    if exit_code != 0:
        return []
    return [int(pid) for pid in stdout.decode().split()]


def _launch_manager_pid(target):
    """Return the PID of the running launch_manager process, or None."""
    pids = _pids_by_comm(target, "launch_manager")
    return pids[0] if pids else None


@add_test_properties(
    fully_verifies=[],
    partially_verifies=[
        "comp_req__lifecycle__launcher_exit_shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_lm_shutdown(
    target, setup_test, assert_test_results, remote_test_dir, test_output_dir
):
    """
    Objective: Verifies that the Launch Manager exits after performing a shutdown
    (stopping all processes it owns) when requested via SIGTERM, and that a
    SIGTERM that arrives while an explicit switch to the "Off" run target is
    already in progress lets that switch to Off continue to completion (it must
    NOT be cancelled and redone).

    The control client activates run_target_a and then explicitly requests a
    switch to the "Off" run target. component_a (part of run_target_a) stalls
    while it is being terminated during that switch, keeping the switch to Off in
    progress. At that point the test sends a SIGTERM to the launch manager process
    (only that process, not the whole group, so the launch manager performs its
    own orderly shutdown).

    Expected Behaviour: The launch manager lets the in-progress switch to Off
    continue - it must NOT cancel the explicit switch to Off and redo it - all the
    processes it owns are stopped, and it exits cleanly.

    The launch manager honours each component's individual shutdown_timeout during
    its own shutdown: it waits for the in-progress transition to Off to complete,
    giving every process up to its configured shutdown_timeout to exit before it
    would be force-terminated. component_a stalls for less than its shutdown_timeout
    while it is being terminated, so it is given time to exit on its own and
    terminates gracefully (producing its XML result) rather than being SIGKILLed.
    """

    new_config_path = str(remote_test_dir / "etc/lm_shutdown_during_switch_to_off.bin")

    a_terminating = remote_test_dir / "component_a_terminating"

    proc = target.execute_async(
        str(remote_test_dir / "launch_manager"),
        args=["-c", new_config_path],
        cwd=str(remote_test_dir),
    )

    try:
        # Wait until the switch to Off is underway: component_a is being terminated
        # (and is now stalling). This is signalled via file a_terminating and is
        # the window in which the shutdown request arrives.
        _wait_for_file(target, a_terminating, proc, timeout_s=10.0)

        # Request shutdown: send SIGTERM to the launch manager process only, so
        # that the launch manager itself stops the processes it owns (rather than
        # the OS terminating the whole process group directly).
        lm_pid = _launch_manager_pid(target)
        assert lm_pid is not None, "Could not find the running launch_manager process"
        logger.info(f"Sending SIGTERM to launch_manager (pid {lm_pid})")
        exit_code, _ = target.execute(f"kill -15 {lm_pid}")
        assert exit_code == 0, "Failed to send SIGTERM to the launch manager"

        # The launch manager must exit after completing its shutdown.
        deadline = time.monotonic() + 10.0
        while proc.is_running() and time.monotonic() < deadline:
            time.sleep(0.1)
        assert not proc.is_running(), (
            f"Launch manager did not exit after SIGTERM. Output:\n{proc.get_output()}"
        )
        assert proc.get_exit_code() == 0, (
            f"Launch manager did not exit cleanly (code {proc.get_exit_code()}). "
            f"Output:\n{proc.get_output()}"
        )

        # The launch manager must have stopped all the processes it owns.
        for binary in ("component_a", "control_client_mock"):
            running = _pids_by_comm(target, binary)
            assert not running, (
                f"'{binary}' is still running (pids {running}) after launch manager shutdown"
            )

        # Core assertion: the explicit switch to Off must be CONTINUED, not
        # cancelled and redone.
        #
        # The switch to Off was already in progress (MainPG in transition to Off)
        # when the SIGTERM arrived, so the SIGTERM-triggered shutdown must simply
        # let it finish. If instead the launch manager cancels that transition, the
        # main process group logs a "kInTransition -> kCancelled" transition after
        # the SIGTERM was received. That must not happen. (The only legitimate
        # cancellation - the Startup -> run_target_a switch - happens before the
        # SIGTERM, so scoping the check to after the SIGTERM marker excludes it.)
        output = proc.get_output()
        if isinstance(output, bytes):
            output = output.decode(errors="replace")

        sigterm_marker = "received SIGTERM"
        marker_index = output.find(sigterm_marker)
        assert marker_index != -1, (
            f"Launch manager never logged receiving the SIGTERM.\nOutput:\n{output}"
        )
        output_after_sigterm = output[marker_index:]

        cancelled_lines = [
            line
            for line in output_after_sigterm.splitlines()
            if "to kCancelled" in line and "MainPG" in line
        ]
        assert not cancelled_lines, (
            "The explicit switch to Off was cancelled and redone by the launch "
            "manager during shutdown instead of being continued. Offending log "
            "line(s):\n" + "\n".join(cancelled_lines) + "\n\n"
            f"Full launch manager output after SIGTERM:\n{output_after_sigterm}"
        )
    finally:
        if proc.is_running():
            proc.stop()

    # Both components are stopped gracefully as part of the switch to Off and
    # produce their XML results: control_client is terminated when the switch to
    # Off begins, and component_a exits within its shutdown_timeout (which the
    # launch manager honours) instead of being force-terminated.
    assert_test_results({"control_client_mock.xml", "component_a.xml"})

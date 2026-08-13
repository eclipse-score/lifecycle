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
    (stopping all processes it owns) when requested via SIGTERM, and that this
    shutdown takes priority over an in-progress run-target switch (the switch is
    cancelled).

    The control client activates run_target_a and then requests a switch to
    run_target_c. component_a (only part of run_target_a) stalls while it is being
    terminated during the switch, keeping the switch in progress. At that point
    the test sends a SIGTERM to the launch manager process (only that process, not
    the whole group, so the launch manager performs its own orderly shutdown).

    Expected Behaviour: The launch manager cancels the pending switch - so
    component_c (only part of run_target_c) is never started - stops all the
    processes it owns, and exits cleanly.
    """

    new_config_path = str(remote_test_dir / "etc/lm_shutdown_during_rt_switch.bin")

    a_terminating = remote_test_dir / "component_a_terminating"
    c_started = remote_test_dir / "component_c_started"

    proc = target.execute_async(
        str(remote_test_dir / "launch_manager"),
        args=["-c", new_config_path],
        cwd=str(remote_test_dir),
    )

    try:
        # Wait until the switch to run_target_c is underway: component_a is being
        # terminated (and is now stalling), so run_target_c has not been activated
        # yet. This state is signalled via file a_terminating.
        # This is the window in which the shutdown request must win.
        _wait_for_file(target, a_terminating, proc, timeout_s=10.0)

        # run_target_c must not have started yet at this point.
        exit_code, _ = target.execute(f"test -f {c_started}")
        # The assertion below could only fail if either the sleep in component_a's termination code is too short
        # or component_a has been killed by launch manager, because it takes too long to react on SIGTERM.
        assert exit_code != 0, (
            "run_target_c was activated before shutdown was requested - this should not happen"
        )

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

        # The pending switch must have been cancelled: run_target_c never activated.
        exit_code, _ = target.execute(f"test -f {c_started}")
        assert exit_code != 0, (
            "run_target_c was activated: the SIGTERM shutdown request must cancel the pending switch"
        )

        # The launch manager must have stopped all the processes it owns.
        for binary in ("component_a", "component_c", "control_client_mock"):
            running = _pids_by_comm(target, binary)
            assert not running, (
                f"'{binary}' is still running (pids {running}) after launch manager shutdown"
            )
    finally:
        if proc.is_running():
            proc.stop()

    # component_c never runs, so it produces no XML result.
    assert_test_results({"mock_control_client.xml", "component_a.xml"})

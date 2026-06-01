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
import time

from tests.utils.testing_utils.setup_test import setup_test
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_examples(target, setup_test, remote_test_dir):
    """
    Objective: Verifies the example demo runs end-to-end without crashing.

    Starts the launch manager, transitions to Running (all example apps start), then back to Startup.
    Expected Behaviour: All run target transitions complete and the launch manager stays alive throughout.
    """
    lm_path = str(remote_test_dir / "launch_manager")
    lmcontrol_path = str(remote_test_dir / "lmcontrol")

    lm_proc = target.execute_async(lm_path, cwd=str(remote_test_dir))

    time.sleep(1.0)
    assert lm_proc.is_running(), "Launch manager exited unexpectedly during Startup"

    exit_code, _ = target.execute(f"{lmcontrol_path} Running")
    assert exit_code == 0, "lmcontrol Running failed"

    time.sleep(2.0)
    assert lm_proc.is_running(), "Launch manager exited unexpectedly during Running"

    for binary in ("cpp_supervised_app", "rust_supervised_app", "cpp_lifecycle_app"):
        rc, _ = target.execute(f"grep -qa '{binary}' /proc/*/cmdline 2>/dev/null")
        assert rc == 0, f"{binary} is not running after transition to Running"

    exit_code, _ = target.execute(f"{lmcontrol_path} Startup")
    assert exit_code == 0, "lmcontrol Startup failed"

    time.sleep(1.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly after returning to Startup"
    )

    res, _ = target.execute(f"kill -15 -{lm_proc.pid()}")
    assert res == 0, "Failed to send SIGTERM to launch manager process group"
    time.sleep(0.5)
    assert not lm_proc.is_running(), "Launch manager did not stop after SIGTERM"

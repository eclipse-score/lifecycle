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
import os
import pytest
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import RunAs, run_test
from tests.utils.testing_utils.test_results import assert_test_results
from tests.utils.testing_utils.realtime_docker import (  # noqa: F401
    docker_configuration,
    require_realtime_scheduling,
)
from attribute_plugin import add_test_properties

logger = logging.getLogger(__name__)

# Set from _NON_ROOT_ID in BUILD, which also writes it into the launch manager configuration.
NON_ROOT_ID = int(os.environ["NON_ROOT_ID"])

# The only capabilities the launch manager needs to apply the sandbox options (see
# ProcessLauncher::setSchedulingAndSecurity) and to signal the components it started:
#   CAP_SETUID / CAP_SETGID - setuid(), setgid(), setgroups() to the configured identity
#   CAP_KILL                - kill() components running under a different uid
#   CAP_SYS_NICE            - sched_setscheduler() with SCHED_FIFO / SCHED_RR
# RLIMIT_* values in the config only lower hard limits, so CAP_SYS_RESOURCE is deliberately absent.
LAUNCH_MANAGER_CAPS = ["setuid", "setgid", "kill", "sys_nice"]


@pytest.fixture(autouse=True)
def hand_test_dir_to_non_root_user(target, setup_test, remote_test_dir):
    """The test tarball is extracted by root; the unprivileged launch manager must be able to read
    its configuration and write result files into its working directory."""
    exit_code, output = target.execute(
        f"chown -R {NON_ROOT_ID}:{NON_ROOT_ID} {remote_test_dir}"
    )
    assert exit_code == 0, f"Couldn't hand over {remote_test_dir}: {output}"


@add_test_properties(
    partially_verifies=[
        "comp_req__launch_man__uid_gid_support",
        "comp_req__launch_man__scheduling_policy",
        "comp_req__launch_man__supplementary_groups",
        "comp_req__launch_man__process_rlimit_support",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_sandbox_options_non_root(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies the launch manager can apply sandbox options when it is not root, holding only
    the capabilities listed in LAUNCH_MANAGER_CAPS.

    The launch manager is started as uid/gid 1000 (via run_test with run_as, which uses 'setpriv'), with those capabilities set as
    ambient so they survive the exec. It then launches the same components as the sandbox_options test.
    Expected Behaviour: All components start with the configured uid, gid, supplementary groups,
    scheduling policy, resource limits and affinity, and the Run Target transition completes.
    """
    exit_code, output = target.execute("id -u")
    assert exit_code == 0 and output.decode().strip() == "0", (
        "setpriv needs to start as root"
    )

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        run_as=RunAs(
            uid=NON_ROOT_ID, gid=NON_ROOT_ID, capabilities=LAUNCH_MANAGER_CAPS
        ),
        args=["-c", str(remote_test_dir / "etc/sandbox_options_non_root.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "sandbox_options_process_a.xml",
            "sandbox_options_process_b.xml",
            "sandbox_options_process_c.xml",
        },
        additional_search_dirs=["/tmp/tests/sandbox_options_non_root", "/tmp"],
    )


def test_bad_sandbox_options_non_root(target, setup_test, remote_test_dir):
    """
    Objective: Verifies the launch manager fails when it is not root and is
    missing CAP_SETUID.

    The launch manager is started as uid/gid 1000 (via run_test with run_as and expect_failure) with
    LAUNCH_MANAGER_CAPS minus "setuid".
    Expected Behaviour: The launch manager falls back and runs fail_test, which creates the test_failed file.
    """
    exit_code, output = target.execute("id -u")
    assert exit_code == 0 and output.decode().strip() == "0", (
        "setpriv needs to start as root"
    )

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        run_as=RunAs(
            uid=NON_ROOT_ID,
            gid=NON_ROOT_ID,
            # run with one capability missing
            capabilities=[cap for cap in LAUNCH_MANAGER_CAPS if cap != "setuid"],
        ),
        expect_failure=True,
        args=["-c", str(remote_test_dir / "etc/sandbox_options_non_root.bin")],
        cwd=str(remote_test_dir),
    )

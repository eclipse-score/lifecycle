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
"""Docker fixtures for integration tests that need real-time scheduling (CAP_SYS_NICE)."""

import logging
import subprocess
import pytest
from score.itf.plugins.core import determine_target_scope

logger = logging.getLogger(__name__)

# Real-time CPU bandwidth to request for the container, in microseconds per cpu-rt-period
# (the daemon default period is 1000000us, so this leaves 5% for non real-time work).
CPU_RT_RUNTIME_US = 950000


def _daemon_supports_cpu_rt_runtime(request) -> bool:
    """Report whether the Docker daemon can hand real-time CPU bandwidth to a container.

    Whether this works is checked by starting a throwaway container with the options the test needs.
    The check fails on hosts using cgroup v2 (where Docker rejects the option) and on cgroup v1 hosts
    whose daemon has no real-time bandwidth to give away, which is exactly when the option has to
    be left out.

    :param request: pytest request, used to read the Docker options of the ITF Docker plugin.
    :return: True if a container can be started with 'cpu_rt_runtime'.
    """
    # Imported here rather than at module level: the package is provided by the ITF Docker plugin,
    # which is not part of the test when it runs against another target (e.g. the local host).
    import docker as pypi_docker

    image = request.config.getoption("docker_image")
    client = pypi_docker.from_env()

    # The probe needs the image, which is normally loaded by the plugin's bootstrap only once the
    # target starts up, i.e. after this fixture has run. Load it here if it is not there yet.
    bootstrap = request.config.getoption("docker_image_bootstrap")
    try:
        client.images.get(image)
    except pypi_docker.errors.ImageNotFound:
        if not bootstrap:
            logger.warning(
                f"Image '{image}' is not available, assuming no real-time bandwidth"
            )
            return False
        subprocess.run([bootstrap], check=True, capture_output=True)

    try:
        client.containers.run(
            image, "true", cpu_rt_runtime=CPU_RT_RUNTIME_US, remove=True
        )
        return True
    except pypi_docker.errors.APIError as error:
        logger.info(
            f"Docker cannot grant real-time CPU bandwidth, continuing without it: {error}"
        )
        return False


@pytest.fixture(scope=determine_target_scope)
def docker_configuration(request):
    """Grant the container CAP_SYS_NICE so the launch manager can apply the
    SCHED_FIFO real-time scheduling policy configured in sandbox_options.in.json.
    Without it, sched_setscheduler() fails with 'Operation not permitted'.

    On kernels built with CONFIG_RT_GROUP_SCHED (e.g. the WSL2 kernel) running
    cgroup v1, CAP_SYS_NICE alone is not enough: the container's cpu cgroup also
    needs real-time bandwidth, otherwise sched_setscheduler() still fails with
    EPERM. Handing out that bandwidth requires the daemon to own some, i.e.
    /etc/docker/daemon.json must contain
    {"cpu-rt-period": 1000000, "cpu-rt-runtime": 950000}.

    Most hosts neither need nor accept the option: on cgroup v2, and on kernels
    without CONFIG_RT_GROUP_SCHED, CAP_SYS_NICE alone is sufficient and passing
    'cpu_rt_runtime' makes the container fail to start. It is therefore only
    requested where it actually works."""
    configuration = {"cap_add": ["SYS_NICE"]}
    if _daemon_supports_cpu_rt_runtime(request):
        configuration["cpu_rt_runtime"] = CPU_RT_RUNTIME_US
    return configuration


@pytest.fixture(autouse=True)
def require_realtime_scheduling(request, target):
    """Fail Docker tests where the container cannot use real-time scheduling.

    This is the case on cgroup v1 hosts with CONFIG_RT_GROUP_SCHED whose daemon has no real-time
    bandwidth configured (see docker_configuration): the launch manager then cannot apply the
    configured SCHED_FIFO/SCHED_RR policies and the managed processes report a mismatch.
    """

    # This check is only for Linux docker-based execution.
    # Skip check for full qemu emulation (the option is undeclared when the QEMU plugin isn't loaded).
    if request.config.getoption("qemu_config", default=None):
        return

    exit_code, output = target.execute("chrt -f 1 true")
    if exit_code != 0:
        pytest.fail(
            "The container cannot use SCHED_FIFO, so the configured scheduling policies cannot "
            "be applied. On a kernel with CONFIG_RT_GROUP_SCHED and cgroup v1, add "
            '{"cpu-rt-period": 1000000, "cpu-rt-runtime": 950000} to /etc/docker/daemon.json and '
            f"restart the Docker daemon. chrt reported: {output}"
        )

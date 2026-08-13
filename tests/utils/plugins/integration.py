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
import json
import logging
import os
import re
import shlex
from pathlib import Path

import pytest

from score.itf.plugins.core import determine_target_scope

logger = logging.getLogger(__name__)

# Sandbox-local directory a crashing process writes its core dump to, plus the
# core_pattern that routes cores there. core_pattern is a global (non-namespaced)
# kernel setting, so it is written from inside the privileged container and
# restored on teardown - the host is not modified permanently.
CORE_DUMP_DIR = "/tmp/score_cores"
CORE_PATTERN = f"{CORE_DUMP_DIR}/core.%e.%p.%s.%t"


def pytest_addoption(parser):
    parser.addoption(
        "--score-test-binary-path",
        action="store",
        default=None,
        help="Space-separated list of local paths to test binaries.",
    )
    parser.addoption(
        "--score-test-remote-directory",
        action="store",
        default=None,
        help="Absolute remote directory path used during test execution.",
    )


@pytest.fixture
def remote_test_dir(request) -> Path:
    """Returns the remote directory path for the current test."""
    return Path(request.config.getoption("--score-test-remote-directory"))


@pytest.fixture
def test_output_dir() -> Path:
    """Returns the Bazel-provided directory for undeclared test outputs."""
    return Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])


@pytest.fixture(scope=determine_target_scope)
def docker_configuration():
    """Run the test container privileged with an unlimited core-file ulimit so
    crashing processes produce core dumps inside the sandbox.

    Overrides the score_itf default (empty dict). Privileged is required because
    core_pattern can only be written from a privileged container; the core
    ulimit is inherited by every ``docker exec``ed process (e.g. the launch
    manager under test)."""
    import docker  # imported lazily: only the docker target has this dependency

    return {
        "privileged": True,
        "ulimits": [docker.types.Ulimit(name="core", soft=-1, hard=-1)],
    }


# Length the kernel truncates a process comm (core_pattern %e) to: TASK_COMM_LEN
# is 16 bytes including the NUL terminator, so 15 usable characters.
_COMM_MAX = 15


def _load_binary_src_map(cores_dir: str):
    """Map each packaged test binary's basename to its execroot-relative source
    path (``bazel-out/<config>/bin/...``), read from the pkg_tar manifest that
    ``integration_test`` builds next to ``environment.tar``.

    This is Bazel's own dest->src mapping, so it resolves aliases and the right
    per-config path automatically. Returns an empty dict on any failure; callers
    then fall back to a ``<path-to-binary>`` placeholder."""
    try:
        tar = os.path.realpath(os.environ["SCORE_TEST_BINARY_PATH"])
        manifest = Path(tar).with_suffix(".manifest")
        if not manifest.is_file():
            # Fall back to reconstructing the manifest path under bazel-out.
            m = re.search(r"^(.*)/bazel-out/([^/]+)/", cores_dir)
            rel = Path(os.environ["SCORE_TEST_BINARY_PATH"]).with_suffix(".manifest")
            manifest = Path(f"{m.group(1)}/bazel-out/{m.group(2)}/bin") / rel
        entries = json.loads(manifest.read_text())
        return {
            os.path.basename(e["dest"]): e["src"]
            for e in entries
            if e.get("type") == "file"
        }
    except Exception:
        logger.warning("Could not load binary manifest for gdb hints", exc_info=True)
        return {}


def _persistent_outputs_dir(undeclared: str) -> str:
    """Translate the in-sandbox TEST_UNDECLARED_OUTPUTS_DIR into the persistent
    path Bazel copies the outputs to (drops the ``.../sandbox/<type>/<n>/``
    segment) so the reported paths survive the test and are copy-pasteable."""
    return re.sub(r"/sandbox/[^/]+/\d+/execroot/", "/execroot/", undeclared)


def _core_dump_report_lines(names, cores_dir: str):
    """Build the crash-dump banner body: the cores location plus a ready-to-run
    gdb command per core dump."""
    m = re.search(r"^(.*)/bazel-out/", cores_dir)
    out_root = m.group(1) if m else ""
    src_map = _load_binary_src_map(cores_dir)

    lines = [
        f"CRASH DUMP HAS BEEN CREATED! See {cores_dir} for details.",
        "",
        "To open it in gdb (build the crashing binary with -c dbg for symbols):",
    ]
    for name in names:
        exe = name.split(".")[1] if name.count(".") >= 1 else name
        # %e is the comm truncated to _COMM_MAX chars, so match on that prefix;
        # bail out to a placeholder unless exactly one binary matches.
        matches = [src for base, src in src_map.items() if base[:_COMM_MAX] == exe]
        binary = f"{out_root}/{matches[0]}" if len(matches) == 1 else "<path-to-binary>"
        lines.append(f'    gdb {binary} "{cores_dir}/{name}"')
    return lines


def _collect_core_dumps(target, test_output_dir: Path):
    """Download any core dumps produced in the sandbox into the Bazel test
    outputs; return the list of downloaded core-dump names."""
    rc, out = target.execute(f"ls -1 {CORE_DUMP_DIR} 2>/dev/null")
    if rc != 0:
        return []
    names = [n for n in out.decode(errors="replace").split() if n]
    if not names:
        return []
    dest_dir = Path(test_output_dir) / "cores"
    dest_dir.mkdir(parents=True, exist_ok=True)
    downloaded = []
    for name in names:
        try:
            target.download(f"{CORE_DUMP_DIR}/{name}", str(dest_dir / name))
            downloaded.append(name)
        except Exception:
            logger.warning(f"Failed to download core dump {name}", exc_info=True)
    return downloaded


@pytest.fixture(autouse=True)
def _capture_core_dumps(request, target, test_output_dir):
    """Route core dumps to a sandbox-local directory for the duration of the
    test, collect any that were produced, then restore the original
    core_pattern.

    Only active for the Docker sandbox. core_pattern is a global kernel setting,
    so it is set from inside the privileged container and reverted afterwards
    (the "container-local + auto-restore" strategy)."""
    if request.config.getoption("docker_image", default=None) is None:
        yield  # not the docker sandbox; nothing to capture
        return

    rc, out = target.execute("cat /proc/sys/kernel/core_pattern")
    original = out.decode(errors="replace").strip("\n") if rc == 0 else None

    rc, _ = target.execute(
        f"mkdir -p {CORE_DUMP_DIR} && chmod 0777 {CORE_DUMP_DIR} && "
        f"printf '%s' {shlex.quote(CORE_PATTERN)} > /proc/sys/kernel/core_pattern"
    )
    enabled = rc == 0
    if not enabled:
        logger.warning(
            "Could not enable sandbox core dumps (core_pattern write failed)"
        )

    try:
        yield
    finally:
        try:
            downloaded = _collect_core_dumps(target, test_output_dir)
            if downloaded:
                cores_dir = f"{_persistent_outputs_dir(str(test_output_dir))}/cores"
                banners = getattr(request.config, "_score_core_dump_banners", [])
                banners.append(_core_dump_report_lines(downloaded, cores_dir))
                request.config._score_core_dump_banners = banners
        finally:
            if enabled and original is not None:
                target.execute(
                    f"printf '%s' {shlex.quote(original)} > /proc/sys/kernel/core_pattern"
                )


def pytest_terminal_summary(terminalreporter, exitstatus, config):
    """Print the crash-dump banner(s) after the FAILURES section, so the pointer
    to each core dump and its gdb command is the last thing shown."""
    banners = getattr(config, "_score_core_dump_banners", None)
    if not banners:
        return
    for lines in banners:
        terminalreporter.write_sep("=", "CRASH DUMP", red=True, bold=True)
        for line in lines:
            terminalreporter.write_line(line)


def pytest_configure(config):
    binary_path = config.getoption("--score-test-binary-path", default=None)
    if binary_path is not None:
        os.environ["SCORE_TEST_BINARY_PATH"] = binary_path

    remote_dir = config.getoption("--score-test-remote-directory", default=None)
    if remote_dir is not None:
        os.environ["SCORE_TEST_REMOTE_DIRECTORY"] = remote_dir

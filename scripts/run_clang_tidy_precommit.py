#!/usr/bin/env python3
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

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run(cmd: list[str], cwd: Path) -> None:
    subprocess.run(cmd, cwd=cwd, check=True)


def sanitize_compile_commands(input_path: Path, output_path: Path) -> None:
    with input_path.open("r", encoding="utf-8") as f:
        db = json.load(f)

    for entry in db:
        args = entry.get("arguments")
        if isinstance(args, list):
            filtered: list[str] = []
            skip_next = False
            for arg in args:
                if skip_next:
                    skip_next = False
                    continue
                if arg == "-z":
                    skip_next = True
                    continue
                if arg.startswith("-z "):
                    continue
                filtered.append(arg)
            entry["arguments"] = filtered

        cmd = entry.get("command")
        if isinstance(cmd, str):
            entry["command"] = cmd.replace(" -z noexecstack", "")

    with output_path.open("w", encoding="utf-8") as f:
        json.dump(db, f)


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    compile_db = repo_root / "compile_commands.json"

    if not compile_db.exists():
        run(
            [
                "bazel",
                "run",
                "//:generate_compile_commands",
                "--",
                "--config=x86_64-linux",
            ],
            cwd=repo_root,
            quiet=True,
        )

    tool = (
        repo_root
        / "bazel-bin"
        / "external"
        / "toolchains_llvm++llvm+llvm_toolchain"
        / "clang-tidy"
    )
    if not (tool.exists() and os_access_executable(tool)):
        run(["bazel", "build", "@llvm_toolchain//:clang-tidy"])

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_compile_db = Path(tmp_dir) / "compile_commands.json"
        sanitize_compile_commands(compile_db, tmp_compile_db)

        cmd = [str(tool), f"-p={tmp_dir}", *sys.argv[1:]]
        completed = subprocess.run(cmd, cwd=repo_root)
        return completed.returncode


def os_access_executable(path: Path) -> bool:
    return path.exists() and bool(path.stat().st_mode & 0o111)


if __name__ == "__main__":
    raise SystemExit(main())

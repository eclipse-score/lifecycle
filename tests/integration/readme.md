<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Local integration testing

## Running the integration tests

To run all tests, simply run `bazel test //tests/integration/... --config=x86_64-linux`

## Config

Currently the following configs are supported:
- `host`
- `x86_64-linux`

## Crash dumps (core dumps)

When a binary under test (e.g. the launch manager) crashes with `SIGSEGV`,
`SIGABRT`, etc. inside the Docker sandbox, a core dump is captured
automatically. This is wired in commonly for every `integration_test`, so
individual tests need no changes.

How it works:
- The sandbox container runs privileged with an unlimited core-file `ulimit`.
- A shared fixture sets the kernel `core_pattern` to a sandbox-local path
  (`/tmp/score_cores/core.%e.%p.%s.%t`), copies any core dumps produced during
  the test into the Bazel test outputs, and then restores the original
  `core_pattern`.

### Getting a crash dump

Run the (crashing) test, disabling the cache so it actually executes:
```
bazel test //tests/integration/<test> --config=x86_64-linux --nocache_test_results
```

If a crash dump was created, a `CRASH DUMP` section is printed right under the
pytest `FAILURES` section at the end of the run (the `x86_64-linux` config
enables `--test_output=errors`, so the failing log is shown automatically):
```
=================================== FAILURES ===================================
...
================================== CRASH DUMP ==================================
CRASH DUMP HAS BEEN CREATED! See <.../test.outputs/cores> for details.

To open it in gdb (build the crashing binary with -c dbg for symbols):
    gdb <.../bin/.../launch_manager> "<.../test.outputs/cores/core.launch_manager.*>"
=========================== short test summary info ============================
```
The printed paths are absolute and copy-pasteable. Core files are named
`core.<exe>.<pid>.<signal>.<time>` (signal `11` = `SIGSEGV`); they are only
produced on an actual crash and can be large (hundreds of MB).

### Analysing a crash dump

`fastbuild` binaries carry limited symbols; build with `-c dbg` for a usable
backtrace:
```
bazel build //score/launch_manager --config=x86_64-linux -c dbg
gdb bazel-out/k8-fastbuild/bin/score/launch_manager/src/daemon/launch_manager \
    "$(bazel info bazel-testlogs)/tests/integration/<test>/<test>/test.outputs/cores/"core.launch_manager.*
# (gdb) bt
```

### Important: the `core_pattern` is a global kernel setting

`core_pattern` is **not** namespaced per container - it is shared with the
host. The fixture therefore changes it globally at the start of a test and
restores the original value on teardown. Two consequences:

- **Run one crashing target at a time when investigating.** Concurrent tests
  race on the shared `core_pattern`, so a parallel run can miss dumps or leave
  a stale value.
- **If a run is force-killed before teardown**, the restore may not run and
  `core_pattern` is left pointing at `/tmp/score_cores/...`. Check and restore
  it manually:
  ```
  cat /proc/sys/kernel/core_pattern
  # On WSL, restore the default handler:
  docker run --rm --privileged debian:bookworm-slim \
    bash -c 'echo "|/wsl-capture-crash %t %E %p %s" > /proc/sys/kernel/core_pattern'
  ```
  The default `core_pattern` differs per system (e.g. `|/wsl-capture-crash ...`
  on WSL, `|/lib/systemd/systemd-coredump ...` on systemd hosts, or plain
  `core`). Check yours *before* the first run so you know the value to restore.


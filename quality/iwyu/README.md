<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Include Check (IWYU)

[include-what-you-use](https://github.com/include-what-you-use/include-what-you-use) checks that
every C++ file includes the headers of the symbols it uses, and nothing more. It runs through
[bazel_iwyu](https://github.com/RealtimeRoboticsGroup/bazel_iwyu), which downloads a prebuilt
IWYU binary, so no local installation is needed.

## Usage

```bash
bazel build --config=iwyu -- //score/... \
    -//score/health_monitor:health_monitoring_rust \
    -//score/health_monitor/src/rust/... \
    -//score/launch_manager:alive_rust \
    -//score/launch_manager:lifecycle_rust \
    -//score/launch_manager/src/alive/src/rust/... \
    -//score/launch_manager/src/lifecycle_client/src/rust/...
```

bazel_iwyu runs on every target that provides `CcInfo`, which includes Rust libraries with C++
dependencies. IWYU cannot analyze `.rs` files, so the Rust packages are excluded from the target
list, together with the aliases that point to them (an aspect applied to an alias also runs on its
`actual` target).

The check uses the LLVM/clang toolchain, because IWYU does not understand the flags of the GCC
toolchain. Do not combine `--config=iwyu` with `--config=x86_64-linux`.

For every source file IWYU writes a report next to the target, for example
`bazel-bin/<package>/<target>.<file>.iwyu.txt`.

## Configuration

- [score_baselibs.imp](score_baselibs.imp) maps implementation headers of score_baselibs to their
  public header, for example `score/result/details/expected/expected.h` to `score/result/result.h`.
  It is passed to IWYU with `--@bazel_iwyu//:iwyu_mappings`, together with the default mappings of
  bazel_iwyu.
- In the code, use `// IWYU pragma: export` when a header is meant to provide another header to its
  users, and `// IWYU pragma: no_include` when IWYU suggests an internal header of a dependency.

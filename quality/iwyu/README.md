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
bazel build --config=iwyu -- $(bazel query 'kind("cc_library|cc_binary", //score/...)')
```

bazel_iwyu runs on every target that provides `CcInfo`, which includes Rust libraries with C++
dependencies. IWYU cannot analyze `.rs` files, so the query selects only the C++ targets.

The check runs in CI for every pull request with the same query (`.github/workflows/_iwyu.yml`).

The check uses the LLVM/clang toolchain, because IWYU does not understand the flags of the GCC
toolchain. Do not combine `--config=iwyu` with `--config=x86_64-linux`.

For every source file IWYU writes a report next to the target, for example
`bazel-bin/<package>/<target>.<file>.iwyu.txt`.

## Configuration

- [score_baselibs.imp](score_baselibs.imp) maps implementation headers of score_baselibs to their
  public header, for example `score/result/details/expected/expected.h` to `score/result/result.h`.
  It is passed to IWYU with `--@bazel_iwyu//:iwyu_mappings`, together with the default mappings of
  bazel_iwyu.

## Pragmas

For IWYU to fully understand the source code some pragmas are needed to explicitly define some behaviors.
For more information look at https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUPragmas.md

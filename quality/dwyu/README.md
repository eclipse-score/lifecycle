<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Bazel Dependency Check (DWYU)

[depend_on_what_you_use](https://github.com/martis42/depend_on_what_you_use) checks that every
C++ target lists the Bazel dependencies of the headers it includes, instead of getting them
through other dependencies. It only runs the preprocessor, so it works with the GCC and QCC
toolchains.

## Usage

```bash
bazel build --config=x86_64-linux --config=dwyu //score/...
```

The build fails for every target with a missing or unused dependency and prints a report.
Only targets of this repository are checked, external dependencies are skipped.
The check runs in CI for every pull request (`dwyu` job in `.github/workflows/on-pr.yml`).

## Configuration

- `target_mapping` in [BUILD](BUILD): some public targets of external modules (for example
  `@score_baselibs//score/os/utils:thread`) only forward headers from targets that are not visible
  to us. The mapping tells DWYU that the public target provides those headers.
- `ignored_unused_deps` in [aspect.bzl](aspect.bzl): targets that are linked only for their side
  effect and are never included, such as the assertion handler.
- To skip a single target, add the tag `dwyu:skip` to it and explain why in a comment.

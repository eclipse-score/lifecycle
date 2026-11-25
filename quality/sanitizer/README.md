<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Sanitizer

Runtime sanitizer builds for launch_manager and health_monitor, using the
shared runtime infrastructure (wrapper, suppression files, env templates) from
`@score_cpp_policies//sanitizers`. The root `.bazelrc` imports the configurations
from `score.bazelrc/score_toolchain.bazelrc` and supplies the local TSan flags.

## Usage

The following commands run all unit and integration tests with sanitizers enabled.

ASan + UBSan + LSan (recommended):

```bash
bazel test --config=asan_ubsan_lsan --config=x86_64-linux //score/... //tests/...
```

ThreadSanitizer (cannot be combined with ASan/LSan):

```bash
bazel test --config=tsan --config=x86_64-linux //score/... //tests/...
```

The `asan`, `ubsan` and `lsan` configs each enable one sanitizer.
The `tsan_ubsan` config enables ThreadSanitizer and UndefinedBehaviorSanitizer together.

## Notes

- Tests can opt out of a sanitizer with the `target_compatible_with` property: `no_asan`, `no_ubsan`,
  `no_lsan` (ASan/UBSan/LSan run) or `no_tsan` (TSan run).
- Sanitizer builds enable minimal debug info (`-g1`) and disable stripping so
  that stack traces resolve to source locations.

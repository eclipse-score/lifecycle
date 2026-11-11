<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Unit test mocks

Mocks in Lifecycle use GMock injected in a few different ways.

## Dependency injection

This is the preferred way to mock functionality for unit tests and can be used in any case where a method or class uses an existing object.

To mock a class `Foo` used in a method `Bar(Foo& param)`

1. Define an interface `IFoo` declaring `Foo`'s public methods
2. Make `Foo` implement `IFoo`
3. Change signatures accepting `Foo` to accept `IFoo`
4. Create a gmock `MockFoo` that also implements `IFoo`
5. Pass an instance of `MockFoo` to the methods under test

## Linker overrides

For syscalls or `extern "C"` free functions, linker overrides are used. The Bazel target for the test must use `linkstatic = True` and a `linkopts` argument, `"-Wl,--wrap=func_to_wrap"`, for each wrapped function.

See [[tests/utils/mocks/mock_syscalls.hpp]] for examples of how to write gmocks for these functions.

## Header replacement

These mocks are needed when construction, destruction, or static methods need to be mocked. They declare a mock header to replace one that source files link against and are usually found in an `injection_mocks` subdirectory. The file name is identical and Bazel's `strip_include_prefix` is used to ensure that mock header paths match the real headers. An additional Bazel target is used that includes the mock targets instead of the real dependencies.

Each header file has an `extern std::unique_ptr` that must be defined and managed within test files to allow use of the mock.
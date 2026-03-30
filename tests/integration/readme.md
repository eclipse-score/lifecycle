# Local integration testing

## Running the tests

To run all tests, simply run `bazel test //tests/integration/... --config=x86_64-linux`

## Config

Currently the following configs are supported:
- `host`
- `x86_64-linux`

## Running a single test

You can run a single integration test locally using `bazel test //tests/integration/<test name>`

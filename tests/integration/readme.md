# Local integration testing

## Prerequisites
- fakechroot must be installed to run these tests
    - `sudo apt install fakechroot`

## Running the tests

To run all tests, simply run `bazel test //tests/integration/...`

## Running a single test
You can run a single integration test locally using `bazel test //tests/integration/<test name>`
# Test helper

## Predefined processes

The following is a list of predefined binaries that integration tests can reuse.

Several tests deploy the same binary as more than one component. These processes read the `PROCESSIDENTIFIER`
environment variable (set per component in the test's JSON configuration) to keep their result and signal files
distinct.

### `reporting_process`

Minimal reporting component. Calls `report_running()` and then stays alive until it receives ``SIGTERM``.

* Args: None
* GTest XML: `reporting_process_<PROCESSIDENTIFIER>`.

### `complex_reporting_process`

Reporting component built on `score::mw::lifecycle::Application` / `run_application`.

* Args: 
Takes one argument, a delay in milliseconds; it sleeps for that long and then
reports running. Exits as soon as its test has run.

### `verification_process`

Not a GTest. Touches signal files according to its arguments:

- `fallback_run_target` touches the fallback file (`fallback_reached`).
- `test_end` sends SIGTERM to its parent (the Launch Manager) to end the test.

Any other argument is an error.

### `process_hanging_on_sigterm`

Touches `<PROCESSIDENTIFIER>_started`, reports running, and waits for SIGTERM. When terminated it signals its parent
with SIGTERM and then stalls for 2 s before exiting. The stall keeps a Run Target switch in progress, so a test has a
deterministic window to act, for example to shut the Launch Manager down mid-switch.

### `gated_process`

Touches `<PROCESSIDENTIFIER>_started` and then waits for the test to create `<PROCESSIDENTIFIER>_release` before it
reports running. This holds a Run Target activation in progress for as long as the test needs. If it is terminated
before it is released, it exits without ever reporting running. See `gated_process.hpp` for the file path helpers.

### `process_crashing_on_startup_n_times`

Aborts on startup the first N times it is launched and starts normally after that. N is the optional first argument
(default 3). The crash count is persisted in `crash_count_<N>`, and each N writes its own XML report
(`..._n_equals_<N>.xml`), so concurrent configurations do not interfere.

### `counting_process`

Not a GTest. Optionally posts to a named POSIX semaphore (`--post-to-semaphore=<name>`) and optionally reports
running (`--report-running`). Exit code 0 on success, 1 on a usage error, 2 if posting failed. Meant to be used with
`waiting_process`.

### `waiting_process`

Not a GTest. Waits until a named POSIX semaphore has been posted `--expected-count` times, for example by several
`counting_process` instances. Options:

- `--semaphore-name`: name of the semaphore, which must start with `/`.
- `--expected-count`: number of posts to wait for.
- `--timeout-seconds`: how long to wait (default 10).
- `--start-from-zero`: delete the semaphore first.
- `--succeed-on-timeout`: exit with 0 if the wait times out.

Exit code 0 on success, 1 on a usage error, 2 on timeout.

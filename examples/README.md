# Demo Setup

## Building & Running the demo setup

Execute `bazel run //examples:run_examples --config=<...>`. This will build all dependences and run the run.sh script

The run.sh script will:

- Build a docker image for execution with the required artifacts inside
- Start the docker container that runs launch_manager

## Interacting with the Demo

### Changing RunTargets

There is a CLI application that allows to request transition to a certain RunTarget.

Example: `lmcontrol Startup`

### Triggering Supervision Failure

There is a CLI command that allows to trigger a supervised app to misreport checkpoints.

Example: `fail <PID>`

## Demo Walkthrough

There is an interactive mode that walks you through two demo scenarios.
This mode requires the run.sh script to be executed **in an active tmux** session.

`bazel run //examples:run_examples --config=<...> -- tmux`

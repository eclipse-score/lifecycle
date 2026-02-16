# Demo Setup

## Building & Running the demo setup

1. Build launch_manager and health_monitor, in the parent folder, first.
2. Then start run.sh script in this folder: `cd demo && ./run.sh`

The run.sh script will:

- Copy the required binaries to a temporary directory demo/tmp
- Compile the json configuration to flatbuffer using flatc
- Build a docker image for execution with the required artifacts inside
- Start the docker container that runs launch_manager

## Interacting with the Demo

### Changing ProcessGroup States

There is a CLI application that allows to request transition to a certain ProcessGroup State.

Example: `lmcontrol ProcessGroup1/Startup`

### Triggering Supervision Failure

There is a CLI command that allows to trigger a supervised app to misreport checkpoints.

Example: `fail <PID>`

## Demo Walkthrough

There is an interactive mode that walks you through two demo scenarios.
This mode requires the run.sh script to be executed **in an active tmux** session.

`cd demo && ./run.sh tmux`

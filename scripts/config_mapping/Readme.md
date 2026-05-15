# Motivation

We are introducing a new, simpler configuration file for the launch_manager.
To make use of the new configuration as early as possible, we are introducing a script to map the new configuration to the old configuration.
Once the source code of the launch_manager has been adapted to read in the new configuration file, the mapping script will become obsolete.

# Usage

Providing a json file using the new configuration format as input, the script will first validate the configuration against its schema. Then it will map the content to the old configuration file format and generate those files into the specified output_dir.

## Bazel

The bazel function `launch_manager_config` handles the translation of the new configuration format into the old configuration format and also does the subsequent compilation to flatbuffer files.

```python
load("@score_lifecycle_health//:defs.bzl", "launch_manager_config")

# This is your launch manager configuration in the new format
exports_files(["lm_config.json"])

# Afterwards, you can refer to the generated flatbuffer files with :example_config_gen
launch_manager_config(
    name ="example_config_gen",
    config="//scripts/config_mapping:lm_config.json"
)
```

## Python

```
python3 lifecycle_config.py <new_configuration.json> -o <output_dir> --schema <path/to/schema>
```

If you want to **only** validate the configuration against its schema without generating any output:

```
python3 lifecycle_config.py <new_configuration.json> --schema <path/to/schema> --validate
```

# Running Tests

```bash
bazel test //scripts/config_mapping/tests:lifecycle_config_tests
```

# Mapping Details

## Mapping of RunTargets to ProcessGroups

The LaunchManager will be configured with only a single Process Group called `MainPG`.
Each RunTarget will be mapped to a ProcessGroupState with the same name.
For example, RunTarget `Minimal` will result in a ProcessGroupState called `MainPG/Minimal`.
The ProcessGroupState will contain all the processes that would be started as part of the associated RunTarget.

The LaunchManager will currently always startup up `MainPG/Startup` as initial state. 
Therefore, we require for now that `initial_run_target` must be set to `Startup`.
This is ensured as part of the translation, configs with `initial_run_target` not equal to `Startup` are rejected.

## Mapping of Components to Processes

There is a 1:1 mapping from Component to Processes.

### Mapping of Deployment Config to Startup Config

There is a 1:1 mapping from deployment config to startup config.
Every Component can only have a single deployment config, therefore the mapped Process configuration will only have a single startup config.

### Mapping of ReadyCondition to Execution Dependencies

The ReadyCondition of a Component is mapped to an execution dependency between two processes.
If Component A has ReadyCondition `process_state:Running` and Component B depends on Component A. Then the ReadyCondition of Component A is mapped to `Component B depends on Component A in State Running`.

For ReadyCondition `process_state:Terminated`, the mapping is only supported for Components that have at least one other Component depending on it. Otherwise, this ReadyCondition cannot be mapped to an execution dependency.

## Mapping of Recovery Actions

The only supported RecoveryAction during startup of a Component is the restart of a Component. This RecoveryAction is mapped to the `restartAttempts` parameter in the old configuration.

For failures after Component startup the only currently supported RecoveryAction is switching to the `fallback_run_target`.
The `fallback_run_target` is mapped to a ProcessGroupState `MainPG/fallback_run_target` and this state will be configured as the recovery state (`ModeGroup/recoveryMode_name`).
This will initiate a transition to the target ProcessGroupState/RunTarget when a process crashes at runtime or a supervision fails. We assume that this transition must not fail.

## Mapping of Alive Supervision

For each Component with application_type `Reporting_And_Supervised` or `State_Manager`, we will create an Alive Supervision configuration. There is a 1:1 mapping from the `component_properties/application_profile/alive_supervision` parameters to the old configuration Alive Supervision structure.

## Mapping of Watchdog Configuration

The new configuration format allows to configure a single watchdog. There is simple 1:1 mapping to the old watchdog configuration format.

## Known Limitations

For a full list of known limitations, refer to the [Known Limitations](../../docs/module/launch_manager_deamon/product_documentation/known_limitations.rst) section of the documentation.



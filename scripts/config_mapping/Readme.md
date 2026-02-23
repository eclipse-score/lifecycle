# Motivation

We are introducing a new, simpler configuration file for the launch_manager.
To make use of the new configuration as early as possible, we are introducing a script to map the new configuration to the old configuration.
Once the source code of the launch_manager has been adapted to read in the new configuration file, the mapping script will become obsolete.

# Usage

Providing a json file using the new configuration format as input, the script will first validate the configuration against its schema. Then it will map the content to the old configuration file format and generate those files into the specified output_dir.

## Bazel

The bazel function `gen_lifecycle_config` handles the translation of the new configuration format into the old configuration format and also does the subsequent compilation to flatbuffer files.

```python
load("@score_lifecycle_health//scripts/config_mapping:config.bzl", "gen_lifecycle_config")

# This is your launch manager configuration in the new format
exports_files(["lm_config.json"])

# Afterwards, you can refer to the generated flatbuffer files with :example_config_gen
gen_lifecycle_config(
    name ="example_config_gen",
    config="//scripts/config_mapping:lm_config.json"
)
```

## Python

```
python3 lifecycle_config.py <new_configuration.json> -o <output_dir>
```

If you want to **only** validate the configuration without generating any output:

```
python3 lifecycle_config.py <new_configuration.json> --validate
```

# Running Tests

You may want to use the virtual environment:

```bash
python3 -m venv myvenv
. myvenv/bin/activate
pip3 install -r requirements_internal.txt
```

Execute tests:

```bash
pytest unit_tests.py
pytest integration_tests.py
```

# Mapping Details

## Mapping of RunTargets to ProcessGroups

The LaunchManager will be configured with only a single Process Group called `MainPG`.
Each RunTarget will be mapped to a ProcessGroupState with the same name.
For example, RunTarget `Minimal` will result in a ProcessGroupState called `MainPG/Minimal`.
The ProcessGroupState will contain all the processes that would be started as part of the associated RunTarget.

The LaunchManager will startup up `MainPG/Startup` by default. Therefore, we require for now that `initial_run_target` must be set to `Startup`.

## Mapping of Components to Processes

There is a 1:1 mapping from Component to Processes.

### Mapping of Deployment Config to Startup Config

There is a 1:1 mapping from deployment config to startup config.
Every Component can only have a single deployment config, therefore the mapped Process configuration will only have a single startup config.

### Mapping of ReadyCondition to Execution Dependencies

The ReadyCondition of a Component is mapped to an execution dependency between two processes.
If Component A has ReadyCondition `process_state:Running` and Component B depends on Component A. Then the ReadyCondition of Component A is mapped to `Component B depends on Component A in State Running`.

For ReadyCondition `process_state:Terminated`, the mapping is only supported for Components that are not directly assigned to a RunTarget. Otherwise, this ReadyCondition cannot be mapped to an execution dependency.

## Mapping of Recovery Actions

The only supported RecoveryAction during startup of a Component is the restart of a Component. This RecoveryAction is mapped to the `restartAttempts` parameter in the old configuration.

The RecoveryAction after component startup (parameter `components/<component>/deployment_config/recovery_action`) as well as the RecoveryAction for RunTargets (parameter `run_targets/<RunTarget>/recovery_action`) are currently not supported.

The `run_targets/final_recovery_action` RecoveryAction will be mapped to the `ModeGroup/recoveryMode_name` parameter. This will initiate a transition to the target ProcessGroupState/RunTarget when a process crashes at runtime or a supervision fails. We assume that this transition must not fail.

## Mapping of Alive Supervision

For each Component with application_type `REPORTING_AND_SUPERVISED` or `STATE_MANAGER`, we will create an Alive Supervision configuration in the old configuration format. There is a 1:1 mapping from the `component_properties/application_profile/alive_supervision` parameters to the old configuration Alive Supervision structure.

## Mapping of Watchdog Configuration

## Known Limitations

* The sandbox parameters `max_memory_usage` and `max_cpu_usage` are currently not supported.
* The initial RunTarget must be named `Startup` and the `initial_run_target` must be configured to `Startup`.
* For ReadyCondition `process_state:Terminated`, the mapping is only supported for Components that are not directly assigned to a RunTarget
* The `ready_recovery_action` only supports the RecoveryAction of type `restart`
* The parameters `components/<component>/deployment_config/recovery_action` and `run_targets/<RunTarget>/recovery_action` are currently not supported. Only the global `final_recovery_action` is supported
* The parameter `run_targets/<RunTarget>/transition_timeout` is currently not supported


Open topics:

* What if an object is explicitly set to {} in the config? Will this overwrite the default to None?
* What about supervision and state manager?
* What is the default ready condition?


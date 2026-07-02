#!/usr/bin/env python3

import argparse
from copy import deepcopy
import json
import os
import sys
from typing import Dict, Any

score_defaults = json.loads("""
{
    "deployment_config": {
        "ready_timeout": 0.5,
        "shutdown_timeout": 0.5,
        "environmental_variables": {},
        "bin_dir": "/opt",
        "working_dir": "/tmp",
        "ready_recovery_action": {
            "restart": {
                "number_of_attempts": 0,
                "delay_before_restart": 0
            }
        },
        "recovery_action": {
            "switch_run_target": {
                "run_target": "fallback_run_target"
            }
        },
        "sandbox": {
            "uid": 1000,
            "gid": 1000,
            "supplementary_group_ids": [],
            "security_policy": "",
            "scheduling_policy": "SCHED_OTHER",
            "scheduling_priority": 0
        }
    },
    "component_properties": {
        "binary_name": "",
        "application_profile": {
            "application_type": "Reporting_And_Supervised",
            "is_self_terminating": false,
            "alive_supervision": {
                "reporting_cycle": 0.5,
                "failed_cycles_tolerance": 2,
                "min_indications": 1,
                "max_indications": 3
            }
        },
        "depends_on": [],
        "process_arguments": [],
        "ready_condition": {
            "process_state": "Running"
        }
    },
    "run_target": {
        "description": "",
        "depends_on": [],
        "transition_timeout": 3,
        "recovery_action": {
            "switch_run_target": {
                "run_target": "fallback_run_target"
            }
        }
    },
    "alive_supervision": {
        "evaluation_cycle": 0.5
    },
    "watchdog": {}
}
""")


def report_error(message):
    print(f"Error: {message}", file=sys.stderr)


# These dictionaries select a single action type (e.g. "restart" vs "switch_run_target").
# When merging, we preserve the user's chosen action type rather than merging across types,
# but we still deep-merge fields within the chosen action type to fill in missing defaults.
_action_selection_dicts = ["ready_recovery_action", "recovery_action"]


def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load and parse a JSON file."""
    with open(file_path, "r") as file:
        return json.load(file)


def preprocess_defaults(global_defaults, config):
    """
    Takes the input configuration and fills in any missing fields with default values.
    The resulting config has no "defaults" entry; all fields are explicit.
    """

    def dict_merge(dict_a, dict_b):
        def dict_merge_recursive(dict_a, dict_b):
            for key, value in dict_b.items():
                if (
                    key in dict_a
                    and isinstance(dict_a[key], dict)
                    and isinstance(value, dict)
                ):
                    if key in _action_selection_dicts:
                        # Replace the action-type selection (don't merge across types),
                        # but still deep-merge fields within the chosen action type.
                        new_val = {}
                        for action_key, action_val in value.items():
                            if (
                                action_key in dict_a[key]
                                and isinstance(dict_a[key][action_key], dict)
                                and isinstance(action_val, dict)
                            ):
                                new_val[action_key] = dict_merge(
                                    dict_a[key][action_key], action_val
                                )
                            else:
                                new_val[action_key] = action_val
                        dict_a[key] = new_val
                    else:
                        dict_a[key] = dict_merge(dict_a[key], value)
                elif key not in dict_a:
                    # Value only exists in dict_b, just add it to dict_a
                    dict_a[key] = value
                else:
                    # For lists, we want to overwrite the content
                    if isinstance(value, list):
                        dict_a[key] = value
                    # For primitive types, we want to take the one from dict_b
                    else:
                        dict_a[key] = value
            return dict_a

        # We are changing the content of dict_a, so we need a deep copy
        return dict_merge_recursive(deepcopy(dict_a), dict_b)

    config_defaults = config.get("defaults", {})
    # Starting with global_defaults, then applying the defaults from the config on top.
    # This is to ensure that any defaults specified in the input config will override the hardcoded defaults in global_defaults.
    merged_defaults = dict_merge(global_defaults, config_defaults)

    new_config = {}
    new_config["components"] = {}
    for component_name, component_config in config.get("components", {}).items():
        # print("Processing component:", component_name)
        new_config["components"][component_name] = {}
        new_config["components"][component_name]["description"] = component_config.get(
            "description", ""
        )
        # Here we start with the merged defaults, then apply the component config on top, so that any fields specified in the component config will override the defaults.
        new_config["components"][component_name]["component_properties"] = dict_merge(
            merged_defaults["component_properties"],
            component_config.get("component_properties", {}),
        )
        new_config["components"][component_name]["deployment_config"] = dict_merge(
            merged_defaults["deployment_config"],
            component_config.get("deployment_config", {}),
        )

    new_config["run_targets"] = {}
    for run_target, run_target_config in config.get("run_targets", {}).items():
        new_config["run_targets"][run_target] = dict_merge(
            merged_defaults["run_target"], run_target_config
        )

    new_config["alive_supervision"] = dict_merge(
        merged_defaults["alive_supervision"], config.get("alive_supervision", {})
    )
    new_config["watchdog"] = dict_merge(
        merged_defaults["watchdog"], config.get("watchdog", {})
    )

    if "initial_run_target" in config:
        new_config["initial_run_target"] = config["initial_run_target"]

    if "fallback_run_target" in config:
        new_config["fallback_run_target"] = dict_merge(
            merged_defaults["run_target"], config["fallback_run_target"]
        )

    return new_config


def custom_validations(config):
    success = True

    if config.get("initial_run_target") != "Startup":
        report_error(
            "initial_run_target must be configured to 'Startup'. Other values are not yet supported yet."
        )
        success = False

    if "Startup" not in config["run_targets"]:
        report_error(
            '"Startup" is a mandatory RunTarget and must be defined in the configuration.'
        )
        success = False

    if "fallback_run_target" in config["run_targets"]:
        report_error(
            'RunTarget name "fallback_run_target" is reserved, please choose a different name.'
        )
        success = False

    for run_target_name, run_target in config["run_targets"].items():
        recovery_action = run_target.get("recovery_action", {})
        if "switch_run_target" not in recovery_action:
            report_error(
                f'RunTarget "{run_target_name}" has an unsupported recovery_action type. '
                "Only switch_run_target is supported for run targets."
            )
            success = False
        elif (
            recovery_action["switch_run_target"].get("run_target")
            != "fallback_run_target"
        ):
            report_error(
                'For any switch_run_target recovery action, the recovery RunTarget must be set to "fallback_run_target".'
            )
            success = False

    if "fallback_run_target" not in config:
        report_error(
            "fallback_run_target is a mandatory configuration but was not found in the config."
        )
        success = False

    watchdog = config.get("watchdog")
    if watchdog:
        missing = [f for f in _WATCHDOG_REQUIRED if f not in watchdog]
        if missing:
            report_error(f"watchdog config is missing required fields: {missing}")
            success = False

    return success


def check_validation_dependency():
    try:
        import jsonschema
    except ImportError:
        print(
            'jsonschema library is not installed. Please install it with "pip install jsonschema" to enable schema validation.'
        )
        return False
    return True


def schema_validation(json_input, schema, config_path=None, schema_path=None):
    try:
        from jsonschema import validate, ValidationError

        validate(json_input, schema)
        print("Schema Validation successful")
        return True
    except ValidationError as err:
        path = " -> ".join(str(p) for p in err.absolute_path)
        location = f" (at: {path})" if path else ""
        config_info = f"Validated Config: {config_path}" if config_path else ""
        schema_info = f"Schema File: {schema_path}" if schema_path else ""
        print(
            f"Error: Schema validation failed{location}: {err.message}\n{config_info}\n{schema_info}",
            file=sys.stderr,
        )
        return False


# Possible exit codes returned from this script
SUCCESS = 0
SCHEMA_VALIDATION_DEPENDENCY_ERROR = 1
SCHEMA_VALIDATION_FAILURE = 2
CUSTOM_VALIDATION_FAILURE = 3


def output_filename(input_path: str) -> str:
    """Derive the output filename from the input filename: <stem>_gen.json."""
    stem = os.path.splitext(os.path.basename(input_path))[0]
    return f"{stem}_gen.json"


_WATCHDOG_REQUIRED = (
    "device_file_path",
    "max_timeout",
    "deactivate_on_shutdown",
    "require_magic_close",
)

SCHED_POLICY_MAP = {
    "SCHED_OTHER": "OTHER",
    "SCHED_FIFO": "FIFO",
    "SCHED_RR": "RR",
}


def is_supervised(application_type):
    return application_type in ("State_Manager", "Reporting_And_Supervised")


def _map_sandbox(sandbox):
    result = {
        "uid": sandbox["uid"],
        "gid": sandbox["gid"],
        "supplementary_group_ids": sandbox.get("supplementary_group_ids", []),
        "security_policy": sandbox.get("security_policy", ""),
        "scheduling_policy": SCHED_POLICY_MAP.get(
            sandbox["scheduling_policy"], sandbox["scheduling_policy"]
        ),
        "scheduling_priority": sandbox["scheduling_priority"],
    }
    if "max_memory_usage" in sandbox:
        result["max_memory_usage"] = sandbox["max_memory_usage"]
    if "max_cpu_usage" in sandbox:
        result["max_cpu_usage"] = sandbox["max_cpu_usage"]
    return result


def _map_deployment_config(deployment, binary_name=""):
    bin_dir = deployment["bin_dir"].rstrip("/")
    executable_path = f"{bin_dir}/{binary_name}" if binary_name else bin_dir
    result = {
        "ready_timeout": deployment["ready_timeout"],
        "shutdown_timeout": deployment["shutdown_timeout"],
        "executable_path": executable_path,
        "working_dir": deployment["working_dir"],
        "sandbox": _map_sandbox(deployment["sandbox"]),
    }

    env_vars = deployment.get("environmental_variables", {})
    if env_vars:
        result["environmental_variables"] = [
            {"key": k, "value": v} for k, v in env_vars.items()
        ]

    ready_recovery = deployment.get("ready_recovery_action", {})
    if restart := ready_recovery.get("restart"):
        result["ready_recovery_action"] = {
            "number_of_attempts": restart["number_of_attempts"],
            "delay_before_restart": restart["delay_before_restart"],
        }

    recovery = deployment.get("recovery_action", {})
    if switch := recovery.get("switch_run_target"):
        result["recovery_action"] = {"run_target": switch["run_target"]}

    return result


def _map_application_profile(profile):
    result = {
        "application_type": profile["application_type"],
        "is_self_terminating": profile["is_self_terminating"],
    }
    if is_supervised(profile["application_type"]):
        if alive := profile.get("alive_supervision"):
            result["alive_supervision"] = {
                "reporting_cycle": alive["reporting_cycle"],
                "failed_cycles_tolerance": alive["failed_cycles_tolerance"],
                "min_indications": alive["min_indications"],
                "max_indications": alive["max_indications"],
            }
    return result


def _map_component_properties(props):
    result = {
        "application_profile": _map_application_profile(props["application_profile"]),
    }
    if (depends_on := props.get("depends_on")) is not None:
        result["depends_on"] = depends_on
    if (process_arguments := props.get("process_arguments")) is not None:
        result["process_arguments"] = process_arguments
    if ready_condition := props.get("ready_condition"):
        result["ready_condition"] = {"process_state": ready_condition["process_state"]}
    return result


def _copy_run_target_optional_fields(source, dest):
    if description := source.get("description", ""):
        dest["description"] = description
    if depends_on := source.get("depends_on", []):
        dest["depends_on"] = depends_on


def _map_run_target(name, run_target):
    switch = run_target.get("recovery_action", {}).get("switch_run_target")
    if switch is None:
        raise ValueError(
            f'RunTarget "{name}" recovery_action must use switch_run_target.'
        )
    result = {
        "name": name,
        "transition_timeout": run_target["transition_timeout"],
        "recovery_action": {"run_target": switch["run_target"]},
    }
    _copy_run_target_optional_fields(run_target, result)
    return result


def _map_fallback_run_target(fallback):
    result = {
        "transition_timeout": fallback["transition_timeout"],
    }
    _copy_run_target_optional_fields(fallback, result)
    return result


def gen_config(output_dir, config, out_filename="launch_manager_config.json"):
    """
    Generate a single JSON file matching new_lm_flatcfg.fbs.

    Input:
        output_dir: Directory where the file is written
        config: Preprocessed configuration with all defaults applied
        out_filename: Name of the output file (default kept for unit tests)
    """
    output = {}

    if "schema_version" in config:
        output["schema_version"] = config["schema_version"]

    output["components"] = [
        {
            "name": name,
            "description": component.get("description", ""),
            "component_properties": _map_component_properties(
                component["component_properties"]
            ),
            "deployment_config": _map_deployment_config(
                component["deployment_config"],
                component["component_properties"].get("binary_name", ""),
            ),
        }
        for name, component in config["components"].items()
    ]

    output["run_targets"] = [
        _map_run_target(name, rt) for name, rt in config["run_targets"].items()
    ]

    output["initial_run_target"] = config["initial_run_target"]

    output["fallback_run_target"] = _map_fallback_run_target(
        config["fallback_run_target"]
    )

    output["alive_supervision"] = {
        "evaluation_cycle": config["alive_supervision"]["evaluation_cycle"]
    }

    if watchdog := config.get("watchdog"):
        missing = [f for f in _WATCHDOG_REQUIRED if f not in watchdog]
        if missing:
            raise ValueError(f"watchdog config is missing required fields: {missing}")
        output["watchdog"] = {f: watchdog[f] for f in _WATCHDOG_REQUIRED}

    with open(f"{output_dir}/{out_filename}", "w") as f:
        json.dump(output, f, indent=4)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("filename", help="The launch_manager configuration file")
    parser.add_argument(
        "--output-dir", "-o", default=".", help="Output directory for generated files"
    )
    parser.add_argument(
        "--validate",
        default=False,
        action="store_true",
        help="Only validate the provided configuration file against the schema and exit.",
    )
    parser.add_argument("--schema", help="Path to the JSON schema file for validation")
    args = parser.parse_args()

    input_config = load_json_file(args.filename)

    if args.schema:
        # User asked for validation explicitly, but the dependency is not installed, we should exit with an error
        if args.validate and not check_validation_dependency():
            exit(SCHEMA_VALIDATION_DEPENDENCY_ERROR)

        # User asked not explicitly for validation, but the dependency is not installed, we should print a warning and continue without validation
        if not check_validation_dependency():
            print(
                "lifecycle_config.py: jsonschema library is not installed. "
                'Please install it with "pip install jsonschema" to enable schema validation.'
            )
            print("Schema validation will be skipped.")
        else:
            json_schema = load_json_file(args.schema)
            validation_successful = schema_validation(
                input_config,
                json_schema,
                config_path=args.filename,
                schema_path=args.schema,
            )
            if not validation_successful:
                exit(SCHEMA_VALIDATION_FAILURE)

            if args.validate:
                exit(SUCCESS)
    else:
        print(
            'No schema provided, skipping validation. Provide the path to the json schema with "--schema <path>" to enable validation.'
        )

    preprocessed_config = preprocess_defaults(score_defaults, input_config)
    if "schema_version" in input_config:
        preprocessed_config["schema_version"] = input_config["schema_version"]
    if not custom_validations(preprocessed_config):
        exit(CUSTOM_VALIDATION_FAILURE)

    try:
        gen_config(args.output_dir, preprocessed_config, output_filename(args.filename))
    except ValueError as e:
        print(f"Error during configuration generation: {e}", file=sys.stderr)
        exit(CUSTOM_VALIDATION_FAILURE)

    return SUCCESS


if __name__ == "__main__":
    exit(main())

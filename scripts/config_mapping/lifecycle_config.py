#!/usr/bin/env python3

import argparse
from copy import deepcopy
import json
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


# There are various dictionaries in the config where only a single entry is allowed.
# We do not want to merge the defaults with the user specified values for these dictionaries.
not_merging_dicts = ["ready_recovery_action", "recovery_action"]


def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load and parse a JSON file."""
    with open(file_path, "r") as file:
        return json.load(file)


def get_recovery_process_group_state(config):
    # Existence has already been validated in the custom_validations function
    return "MainPG/fallback_run_target"


def sec_to_ms(sec: float) -> int:
    return int(sec * 1000)


def preprocess_defaults(global_defaults, config):
    """
    This function takes the input configuration and fills in any missing fields with default values.
    The resulting file with have no "defaults" entry anymore, but looks like if the user had specified all the fields explicitly.
    """

    def dict_merge(dict_a, dict_b):
        def dict_merge_recursive(dict_a, dict_b):
            for key, value in dict_b.items():
                if (
                    key in dict_a
                    and isinstance(dict_a[key], dict)
                    and isinstance(value, dict)
                ):
                    # For certain dictionaries, we do not want to merge the defaults with the user specified values
                    if key in not_merging_dicts:
                        dict_a[key] = value
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
    components = config.get("components", {})
    for component_name, component_config in components.items():
        # print("Processing component:", component_name)
        new_config["components"][component_name] = {}
        new_config["components"][component_name]["description"] = component_config.get(
            "description", ""
        )
        # Here we start with the merged defaults, then apply the component config on top, so that any fields specified in the component config will override the defaults.
        new_config["components"][component_name]["component_properties"] = dict_merge(
            merged_defaults["component_properties"],
            component_config.get("component_properties"),
        )
        new_config["components"][component_name]["deployment_config"] = dict_merge(
            merged_defaults["deployment_config"],
            component_config.get("deployment_config", {}),
        )

        # Special case:
        # If the defaults specify alive_supervision for component, but the component config sets the type to anything other than "SUPERVISED", then we should not apply the
        # alive_supervision defaults to that component, since it doesn't make sense to have alive_supervision from the defaults.
        # TODO

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

    for key in ("initial_run_target", "fallback_run_target"):
        if key in config:
            new_config[key] = config[key]

    # print(json.dumps(new_config, indent=4))

    return new_config


def gen_health_monitor_config(output_dir, config):
    """
    This function generates the health monitor configuration file based on the input configuration.
    Input:
        output_dir: The directory where the generated files should be saved
        config: The preprocessed configuration in the new format, with all defaults applied

    Output:
    - A file named "hm_demo.json" containing the health monitor daemon configuration
    - A optional file named "hmcore.json" containing the watchdog configuration
    - For each supervised process, a file named "<process>_<process_group>.json"
    """

    def get_process_type(application_type):
        if application_type == "State_Manager":
            return "STM_PROCESS"
        else:
            return "REGULAR_PROCESS"

    def is_supervised(application_type):
        return (
            application_type == "State_Manager"
            or application_type == "Reporting_And_Supervised"
        )

    def get_all_process_group_states(run_targets):
        process_group_states = []
        for run_target, _ in run_targets.items():
            process_group_states.append("MainPG/" + run_target)
        process_group_states.append("MainPG/fallback_run_target")
        return process_group_states

    def get_all_refProcessGroupStates(run_targets):
        states = get_all_process_group_states(run_targets)
        refProcessGroupStates = []
        for state in states:
            refProcessGroupStates.append({"identifier": state})
        return refProcessGroupStates

    HM_SCHEMA_VERSION_MAJOR = 8
    HM_SCHEMA_VERSION_MINOR = 0
    hm_config = {}
    hm_config["versionMajor"] = HM_SCHEMA_VERSION_MAJOR
    hm_config["versionMinor"] = HM_SCHEMA_VERSION_MINOR
    hm_config["process"] = []
    hm_config["hmMonitorInterface"] = []
    hm_config["hmSupervisionCheckpoint"] = []
    hm_config["hmAliveSupervision"] = []
    hm_config["hmDeadlineSupervision"] = []
    hm_config["hmLogicalSupervision"] = []
    hm_config["hmLocalSupervision"] = []
    hm_config["hmGlobalSupervision"] = []
    hm_config["hmRecoveryNotification"] = []
    index = 0
    for component_name, component_config in config["components"].items():
        if is_supervised(
            component_config["component_properties"]["application_profile"][
                "application_type"
            ]
        ):
            process = {}
            process["index"] = index
            process["shortName"] = component_name
            process["identifier"] = component_name
            process["processType"] = get_process_type(
                component_config["component_properties"]["application_profile"][
                    "application_type"
                ]
            )
            process["refProcessGroupStates"] = get_all_refProcessGroupStates(
                config["run_targets"]
            )
            process["processExecutionErrors"] = [
                {"processExecutionError": 1} for _ in process["refProcessGroupStates"]
            ]
            hm_config["process"].append(process)

            hmMonitorIf = {}
            hmMonitorIf["instanceSpecifier"] = component_name
            hmMonitorIf["processShortName"] = component_name
            hmMonitorIf["portPrototype"] = "DefaultPort"
            hmMonitorIf["interfacePath"] = "lifecycle_health_" + component_name
            hmMonitorIf["refProcessIndex"] = index
            hmMonitorIf["permittedUid"] = component_config["deployment_config"][
                "sandbox"
            ]["uid"]
            hm_config["hmMonitorInterface"].append(hmMonitorIf)

            checkpoint = {}
            checkpoint["shortName"] = component_name + "_checkpoint"
            checkpoint["checkpointId"] = 1
            checkpoint["refInterfaceIndex"] = index
            hm_config["hmSupervisionCheckpoint"].append(checkpoint)

            alive_supervision = {}
            alive_supervision["ruleContextKey"] = component_name + "_alive_supervision"
            alive_supervision["refCheckPointIndex"] = index
            alive_supervision["aliveReferenceCycle"] = sec_to_ms(
                component_config["component_properties"]["application_profile"][
                    "alive_supervision"
                ]["reporting_cycle"]
            )
            alive_supervision["minAliveIndications"] = component_config[
                "component_properties"
            ]["application_profile"]["alive_supervision"]["min_indications"]
            alive_supervision["maxAliveIndications"] = component_config[
                "component_properties"
            ]["application_profile"]["alive_supervision"]["max_indications"]
            alive_supervision["isMinCheckDisabled"] = (
                alive_supervision["minAliveIndications"] == 0
            )
            alive_supervision["isMaxCheckDisabled"] = (
                alive_supervision["maxAliveIndications"] == 0
            )
            alive_supervision["failedSupervisionCyclesTolerance"] = component_config[
                "component_properties"
            ]["application_profile"]["alive_supervision"]["failed_cycles_tolerance"]
            alive_supervision["refProcessIndex"] = index
            alive_supervision["refProcessGroupStates"] = get_all_refProcessGroupStates(
                config["run_targets"]
            )
            hm_config["hmAliveSupervision"].append(alive_supervision)

            local_supervision = {}
            local_supervision["ruleContextKey"] = component_name + "_local_supervision"
            local_supervision["infoRefInterfacePath"] = ""
            local_supervision["hmRefAliveSupervision"] = [
                {"refAliveSupervisionIdx": index}
            ]
            hm_config["hmLocalSupervision"].append(local_supervision)

            with open(
                f"{output_dir}/hmproc_{component_name}.json", "w"
            ) as process_file:
                process_config = {}
                process_config["versionMajor"] = HM_SCHEMA_VERSION_MAJOR
                process_config["versionMinor"] = HM_SCHEMA_VERSION_MINOR
                process_config["process"] = []
                process_config["hmMonitorInterface"] = []
                process_config["hmMonitorInterface"].append(hmMonitorIf)
                json.dump(process_config, process_file, indent=4)

            index += 1

    indices = [i for i in range(index)]
    if len(indices) > 0:
        # Create one global supervision & recovery action for all processes.
        global_supervision = {}
        global_supervision["ruleContextKey"] = "global_supervision"
        global_supervision["isSeverityCritical"] = False
        global_supervision["localSupervision"] = [
            {"refLocalSupervisionIndex": idx} for idx in indices
        ]
        global_supervision["refProcesses"] = [{"index": idx} for idx in indices]
        global_supervision["refProcessGroupStates"] = get_all_refProcessGroupStates(
            config["run_targets"]
        )
        hm_config["hmGlobalSupervision"].append(global_supervision)

        recovery_action = {}
        recovery_action["shortName"] = f"recovery_notification"
        recovery_action["recoveryNotificationTimeout"] = 5000
        recovery_action["processGroupMetaModelIdentifier"] = (
            get_recovery_process_group_state(config)
        )
        recovery_action["refGlobalSupervisionIndex"] = hm_config[
            "hmGlobalSupervision"
        ].index(global_supervision)
        recovery_action["instanceSpecifier"] = ""
        recovery_action["shouldFireWatchdog"] = False
        hm_config["hmRecoveryNotification"].append(recovery_action)

    with open(f"{output_dir}/hm_demo.json", "w") as hm_file:
        json.dump(hm_config, hm_file, indent=4)

    HM_CORE_SCHEMA_VERSION_MAJOR = 3
    HM_CORE_SCHEMA_VERSION_MINOR = 0
    hmcore_config = {}
    hmcore_config["versionMajor"] = HM_CORE_SCHEMA_VERSION_MAJOR
    hmcore_config["versionMinor"] = HM_CORE_SCHEMA_VERSION_MINOR
    hmcore_config["watchdogs"] = []
    hmcore_config["config"] = [
        {
            "periodicity": sec_to_ms(
                config.get("alive_supervision", {}).get("evaluation_cycle", 0.01)
            )
        }
    ]

    if watchdog_config := config.get("watchdog", {}):
        watchdog = {}
        watchdog["shortName"] = "watchdog"
        watchdog["deviceFilePath"] = watchdog_config["device_file_path"]
        watchdog["maxTimeout"] = sec_to_ms(watchdog_config["max_timeout"])
        watchdog["deactivateOnShutdown"] = watchdog_config["deactivate_on_shutdown"]
        watchdog["hasValueDeactivateOnShutdown"] = True
        watchdog["requireMagicClose"] = watchdog_config["require_magic_close"]
        watchdog["hasValueRequireMagicClose"] = True
        hmcore_config["watchdogs"].append(watchdog)

    with open(f"{output_dir}/hmcore.json", "w") as hm_file:
        json.dump(hmcore_config, hm_file, indent=4)


def gen_launch_manager_config(output_dir, config):
    """
    This function generates the launch manager configuration file based on the input configuration.
    Input:
        output_dir: The directory where the generated files should be saved
        config: The preprocessed configuration in the new format, with all defaults applied

    Output:
    - A file named "lm_demo.json" containing the launch manager configuration
    """

    """
    Recursively get all components on which the run target depends
    """

    def format_dependency_path(path, cycle_target):
        """Format a dependency resolution path for display, highlighting the cycle."""
        return " -> ".join(path + [cycle_target])

    def get_process_dependencies(
        run_target, ancestors_run_targets=None, ancestors_components=None
    ):
        """
        Resolve all component dependencies for the given run target.

        ancestors_run_targets and ancestors_components track the current
        recursion path to detect cyclic dependencies without rejecting
        legitimate diamond-shaped dependency trees.
        """
        if ancestors_run_targets is None:
            ancestors_run_targets = []
        if ancestors_components is None:
            ancestors_components = []

        out = []
        if "depends_on" not in run_target:
            return out

        for dependency_name in run_target["depends_on"]:
            if dependency_name in config["components"]:
                if dependency_name in ancestors_components:
                    path = format_dependency_path(ancestors_components, dependency_name)
                    raise ValueError(
                        f"Cyclic dependency detected: component '{dependency_name}' "
                        f"has already been visited.\n  Path: {path}"
                    )
                ancestors_components.append(dependency_name)
                out.append(dependency_name)

                component_props = config["components"][dependency_name][
                    "component_properties"
                ]
                if "depends_on" in component_props:
                    # All dependencies must be components, since components can't depend on run targets
                    for dep in component_props["depends_on"]:
                        if dep not in config["components"]:
                            raise ValueError(
                                f"Component '{dependency_name}' depends on unknown component '{dep}'."
                            )
                        if dep in ancestors_components:
                            path = format_dependency_path(ancestors_components, dep)
                            raise ValueError(
                                f"Cyclic dependency detected: component '{dependency_name}' "
                                f"depends on already visited component '{dep}'.\n  Path: {path}"
                            )
                        ancestors_components.append(dep)
                        out.append(dep)
                        out += get_process_dependencies(
                            config["components"][dep]["component_properties"],
                            ancestors_run_targets=ancestors_run_targets,
                            ancestors_components=ancestors_components,
                        )
                        ancestors_components.pop()

                ancestors_components.pop()
            else:
                # If the dependency is not a component, it must be a run target
                if dependency_name not in config["run_targets"]:
                    raise ValueError(
                        f"Run target depends on unknown run target or component '{dependency_name}'."
                    )
                if dependency_name in ancestors_run_targets:
                    path = format_dependency_path(
                        ancestors_run_targets, dependency_name
                    )
                    raise ValueError(
                        f"Cyclic dependency detected: run target '{dependency_name}' "
                        f"has already been visited.\n  Path: {path}"
                    )
                ancestors_run_targets.append(dependency_name)
                out += get_process_dependencies(
                    config["run_targets"][dependency_name],
                    ancestors_run_targets=ancestors_run_targets,
                    ancestors_components=ancestors_components,
                )
                ancestors_run_targets.pop()
        return list(set(out))  # Remove duplicates

    def get_terminating_behavior(component_config):
        if component_config["component_properties"]["application_profile"][
            "is_self_terminating"
        ]:
            return "ProcessIsSelfTerminating"
        else:
            return "ProcessIsNotSelfTerminating"

    lm_config = {}
    lm_config["versionMajor"] = 7
    lm_config["versionMinor"] = 0
    lm_config["Process"] = []
    lm_config["ModeGroup"] = [
        {
            "identifier": "MainPG",
            "initialMode_name": "not-used",
            "recoveryMode_name": get_recovery_process_group_state(config),
            "modeDeclaration": [],
        }
    ]

    process_group_states = {}

    # For each component, store which run targets depends on it
    for pgstate, values in config["run_targets"].items():
        state_name = "MainPG/" + pgstate
        lm_config["ModeGroup"][0]["modeDeclaration"].append({"identifier": state_name})
        components = get_process_dependencies(values)
        for component in components:
            if component not in process_group_states:
                process_group_states[component] = []
            process_group_states[component].append(state_name)

    if fallback := config.get("fallback_run_target", {}):
        lm_config["ModeGroup"][0]["modeDeclaration"].append(
            {"identifier": "MainPG/fallback_run_target"}
        )
        fallback_components = get_process_dependencies(fallback)
        for component in fallback_components:
            if component not in process_group_states:
                process_group_states[component] = []
            process_group_states[component].append("MainPG/fallback_run_target")

    for component_name, component_config in config["components"].items():
        process = {}
        process["identifier"] = component_name
        process["path"] = (
            f"{component_config['deployment_config']['bin_dir']}/{component_config['component_properties']['binary_name']}"
        )
        process["uid"] = component_config["deployment_config"]["sandbox"]["uid"]
        process["gid"] = component_config["deployment_config"]["sandbox"]["gid"]
        process["sgids"] = [
            {"sgid": sgid}
            for sgid in component_config["deployment_config"]["sandbox"][
                "supplementary_group_ids"
            ]
        ]
        process["securityPolicyDetails"] = component_config["deployment_config"][
            "sandbox"
        ]["security_policy"]
        process["numberOfRestartAttempts"] = component_config["deployment_config"][
            "ready_recovery_action"
        ]["restart"]["number_of_attempts"]

        match component_config["component_properties"]["application_profile"][
            "application_type"
        ]:
            case "Native":
                process["executable_reportingBehavior"] = "DoesNotReportExecutionState"
            case "State_Manager":
                process["executable_reportingBehavior"] = "ReportsExecutionState"
                process["functionClusterAffiliation"] = "STATE_MANAGEMENT"
            case "Reporting" | "Reporting_And_Supervised":
                process["executable_reportingBehavior"] = "ReportsExecutionState"

        process["startupConfig"] = [{}]
        process["startupConfig"][0]["executionError"] = "1"
        process["startupConfig"][0]["identifier"] = f"{component_name}_startup_config"
        process["startupConfig"][0]["enterTimeoutValue"] = sec_to_ms(
            component_config["deployment_config"]["ready_timeout"]
        )
        process["startupConfig"][0]["exitTimeoutValue"] = sec_to_ms(
            component_config["deployment_config"]["shutdown_timeout"]
        )
        process["startupConfig"][0]["schedulingPolicy"] = component_config[
            "deployment_config"
        ]["sandbox"]["scheduling_policy"]
        process["startupConfig"][0]["schedulingPriority"] = str(
            component_config["deployment_config"]["sandbox"]["scheduling_priority"]
        )
        process["startupConfig"][0]["terminationBehavior"] = get_terminating_behavior(
            component_config
        )
        process["startupConfig"][0]["processGroupStateDependency"] = []
        process["startupConfig"][0]["environmentVariable"] = []
        for env_var, value in (
            component_config["deployment_config"]
            .get("environmental_variables", {})
            .items()
        ):
            process["startupConfig"][0]["environmentVariable"].append(
                {"key": env_var, "value": value}
            )

        if arguments := component_config["component_properties"].get(
            "process_arguments", []
        ):
            arguments = [{"argument": arg} for arg in arguments]
        process["startupConfig"][0]["processArgument"] = arguments

        if component_name in process_group_states:
            for pgstate in process_group_states[component_name]:
                process["startupConfig"][0]["processGroupStateDependency"].append(
                    {"stateMachine_name": "MainPG", "stateName": pgstate}
                )

        lm_config["Process"].append(process)

    # Execution dependencies. Assumption: Components can never depend on run targets
    for process in lm_config["Process"]:
        process["startupConfig"][0]["executionDependency"] = []
        for dependency in config["components"][process["identifier"]][
            "component_properties"
        ].get("depends_on", []):
            dep_entry = config["components"][dependency]
            ready_condition = dep_entry["component_properties"]["ready_condition"][
                "process_state"
            ]
            process["startupConfig"][0]["executionDependency"].append(
                {"stateName": ready_condition, "targetProcess_identifier": dependency}
            )

    with open(f"{output_dir}/lm_demo.json", "w") as lm_file:
        json.dump(lm_config, lm_file, indent=4)


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

    # Check that for any switch_run_target recovery action, the run_target is set to "fallback_run_target"
    for _, run_target in config["run_targets"].items():
        recovery_target_name = (
            run_target.get("recovery_action", {})
            .get("switch_run_target", {})
            .get("run_target", "fallback_run_target")
        )
        if recovery_target_name != "fallback_run_target":
            report_error(
                'For any switch_run_target recovery action, the recovery RunTarget must be set to "fallback_run_target".'
            )
            success = False

    if "fallback_run_target" not in config:
        report_error(
            "fallback_run_target is a mandatory configuration but was not found in the config."
        )
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
                'lifecycle_config.py:jsonschema library is not installed. Please install it with "pip install jsonschema" to enable schema validation.'
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
    if not custom_validations(preprocessed_config):
        exit(CUSTOM_VALIDATION_FAILURE)

    try:
        gen_health_monitor_config(args.output_dir, preprocessed_config)
        gen_launch_manager_config(args.output_dir, preprocessed_config)
    except ValueError as e:
        print(f"Error during configuration generation: {e}", file=sys.stderr)
        exit(CUSTOM_VALIDATION_FAILURE)

    return SUCCESS


if __name__ == "__main__":
    exit(main())

#!/usr/bin/env python3

import argparse
from copy import deepcopy
import json
from typing import Dict, Any

score_defaults = json.loads('''
{
    "deployment_config": {
        "ready_timeout": 0.5,
        "shutdown_timeout": 0.5,
        "environmental_variables": {},
        "bin_dir": "/opt",
        "working_dir": "/tmp",
        "ready_recovery_action": {
            "restart": {
                "number_of_attempts": 1,
                "delay_before_restart": 0.5
            }
        },
        "recovery_action": {
            "switch_run_target": {
                "run_target": "Off"
            }
        },
        "sandbox": {
            "uid": 0,
            "gid": 0,
            "supplementary_group_ids": [],
            "security_policy": "",
            "scheduling_policy": "SCHED_OTHER",
            "scheduling_priority": 0
        }
    },
    "component_properties": {
        "application_profile": {
            "application_type": "REPORTING",
            "is_self_terminating": false
        },
        "ready_condition": {
            "process_state": "Running"
        }
    },
    "run_target": {
        "transition_timeout": 5,
        "recovery_action": {
            "switch_run_target": {
                "run_target": "Off"
            }
        }
    },
    "alive_supervision" : {
        "evaluation_cycle": 0.5
    },
    "watchdogs": {}
}
''')

# There are various dictionaries in the config where only a single entry is allowed.
# We do not want to merge the defaults with the user specified values for these dictionaries.
not_merging_dicts = ["ready_recovery_action", "recovery_action", "final_recovery_action"]

def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load and parse a JSON file."""
    with open(file_path, 'r') as file:
        return json.load(file)

def preprocess_defaults(global_defaults, config):
    """
    This function takes the input configuration and fills in any missing fields with default values.
    The resulting file with have no "defaults" entry anymore, but looks like if the user had specified all the fields explicitly.
    """
    def dict_merge(dict_a, dict_b):
        def dict_merge_recursive(dict_a, dict_b):
            for key, value in dict_b.items():
                if key in dict_a and isinstance(dict_a[key], dict) and isinstance(value, dict):
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
                        dict_a[key] = (value)
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
        #print("Processing component:", component_name)
        new_config["components"][component_name] = {}
        new_config["components"][component_name]["description"] = component_config.get("description", "")
        # Here we start with the merged defaults, then apply the component config on top, so that any fields specified in the component config will override the defaults.
        new_config["components"][component_name]["component_properties"] = dict_merge(merged_defaults["component_properties"], component_config.get("component_properties"))
        new_config["components"][component_name]["deployment_config"] = dict_merge(merged_defaults["deployment_config"], component_config.get("deployment_config", {}))

        # Special case:
        # If the defaults specify alive_supervision for component, but the component config sets the type to anything other than "SUPERVISED", then we should not apply the
        # alive_supervision defaults to that component, since it doesn't make sense to have alive_supervision from the defaults.
        # TODO

    new_config["run_targets"] = {}
    for run_target, run_target_config in config.get("run_targets", {}).items():
        # TODO: initial_run_target is not a dictionary, merging defautls not working for this currently
        if run_target == "initial_run_target":
            new_config["run_targets"][run_target] = run_target_config
        else:
            new_config["run_targets"][run_target] = dict_merge(merged_defaults["run_target"], run_target_config)

    new_config["alive_supervision"] = dict_merge(merged_defaults["alive_supervision"], config.get("alive_supervision", {}))
    new_config["watchdogs"] = dict_merge(merged_defaults["watchdogs"], config.get("watchdogs", {}))

    #print(json.dumps(new_config, indent=4))

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
        if application_type == "STATE_MANAGER":
            return "STATE_MANAGEMENT"
        else:
            return "REGULAR_PROCESS"

    def is_supervised(application_type):
        return application_type == "STATE_MANAGER" or application_type == "REPORTING_AND_SUPERVISED"

    hm_config = {}
    hm_config["versionMajor"] = 8
    hm_config["versionMinor"] = 0
    hm_config["process"]= []
    hm_config["hmMonitorInterface"] = []
    hm_config["hmSupervisionCheckpoint"] = []
    hm_config["hmAliveSupervision"] = []
    hm_config["hmDeadlineSupervision"] = []
    hm_config["hmLogicalSupervision"] = []
    hm_config["hmLocalSupervision"] = []
    index = 0
    for component_name, component_config in config["components"].items():
        if is_supervised(component_config["component_properties"]["application_profile"]["application_type"]):
            process = {}
            process["index"] = index
            process["shortName"] = component_name
            process["identifier"] = component_name
            process["processType"] = get_process_type(component_config["component_properties"]["application_profile"]["application_type"])
            process["refProcessGroupStates"] = [] # TODO, Need to know all RunTargets where this process runs
            process["processExecutionErrors"] = {"processExecutionError":1}
            hm_config["process"].append(process)

            hmMonitorIf = {}
            hmMonitorIf["instanceSpecifier"] = component_name
            hmMonitorIf["processShortName"] = component_name
            hmMonitorIf["portPrototype"] = "DefaultPort"
            hmMonitorIf["interfacePath"] = "lifecycle_health" + component_name
            hmMonitorIf["refProcessIndex"] = index
            hmMonitorIf["permittedUid"] = component_config["deployment_config"]["sandbox"]["uid"]
            hm_config["hmMonitorInterface"].append(hmMonitorIf)

            checkpoint = {}
            checkpoint["shortName"] = component_name + "_checkpoint"
            checkpoint["checkpointId"] = 1
            checkpoint["refInterfaceIndex"] = index
            hm_config["hmSupervisionCheckpoint"].append(checkpoint)

            alive_supervision = {}
            alive_supervision["ruleContextKey"] = component_name + "_alive_supervision"
            alive_supervision["refCheckPointIndex"] = index
            alive_supervision["aliveReferenceCycle"] = component_config["component_properties"]["application_profile"]["alive_supervision"]["reporting_cycle"]
            alive_supervision["minAliveIndications"] = component_config["component_properties"]["application_profile"]["alive_supervision"]["min_indications"]
            alive_supervision["maxAliveIndications"] = component_config["component_properties"]["application_profile"]["alive_supervision"]["max_indications"]
            alive_supervision["isMinCheckDisabled"] = alive_supervision["minAliveIndications"] == 0
            alive_supervision["isMaxCheckDisabled"] = alive_supervision["maxAliveIndications"] == 0
            alive_supervision["failedSupervisionCyclesTolerance"] = component_config["component_properties"]["application_profile"]["alive_supervision"]["failed_cycles_tolerance"]
            alive_supervision["refProcessIndex"] = index
            alive_supervision["refProcessGroupStates"] = [] # TODO, Need to know all RunTargets where this process runs
            hm_config["hmAliveSupervision"].append(alive_supervision)

            local_supervision = {}
            local_supervision["ruleContextKey"] = component_name + "_local_supervision"
            local_supervision["infoRefInterfacePath"] = ""
            local_supervision["hmRefAliveSupervision"] = []
            local_supervision["hmRefAliveSupervision"].append({"refAliveSupervisionIdx": index})
            hm_config["hmLocalSupervision"].append(local_supervision)

            #with open(f"{output_dir}/{process_name}_{process_group}.json", "w") as process_file:
            #    json.dump(process_config, process_file, indent=4)

            index += 1

    # TODO: Add global supervision
    # TODO: Add RecoveryAction

    with open(f"{output_dir}/hm_demo.json", "w") as hm_file:
        json.dump(hm_config, hm_file, indent=4)

def gen_launch_manager_config(output_dir, config):
    """
    This function generates the launch manager configuration file based on the input configuration.
    Input:
        output_dir: The directory where the generated files should be saved
        config: The preprocessed configuration in the new format, with all defaults applied 

    Output:
    - A file named "lm_demo.json" containing the launch manager configuration
    """
    
    with open(f"{output_dir}/lm_demo.json", "w") as lm_file:
        lm_config = {}
        json.dump(lm_config, lm_file, indent=4)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("filename", help="The launch_manager configuration file")
    parser.add_argument("--output-dir", "-o", default=".", help="Output directory for generated files")
    args = parser.parse_args()

    input_config = load_json_file(args.filename)
    preprocessed_config = preprocess_defaults(score_defaults, input_config)
    gen_health_monitor_config(args.output_dir, preprocessed_config)
    gen_launch_manager_config(args.output_dir, preprocessed_config)
    
    return 0

if __name__ == "__main__":
    exit(main())
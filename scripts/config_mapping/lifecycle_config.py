#!/usr/bin/env python3

import argparse
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
    "watchdog": {}
}
''')


def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load and parse a JSON file."""
    with open(file_path, 'r') as file:
        return json.load(file)


def preprocess_defaults(config):
    """
    This function takes the input configuration and fills in any missing fields with default values.
    The resulting file with have no "defaults" entry anymore, but looks like if the user had specified all the fields explicitly.
    """
    def dict_merge(dict_a, dict_b):
        for key, value in dict_b.items():
            if key in dict_a and isinstance(dict_a[key], dict) and isinstance(value, dict):
                dict_a[key] = dict_merge(dict_a[key], value)
            elif key not in dict_a:
                # Value only exists in dict_b, just add it to dict_a
                dict_a[key] = value
            else:
                # For lists, we want to merge the content
                if isinstance(value, list):
                    dict_a[key].extend(value)
                # For primitive types, we want to take the one from dict_b
                else:
                    dict_a[key] = value
        return dict_a

    config_defaults = config.get("defaults", {})
    # Starting with score_defaults, then applying the defaults from the config on top.
    # This is to ensure that any defaults specified in the input config will override the hardcoded defaults in score_defaults.
    merged_defaults = dict_merge(score_defaults, config_defaults)

    new_config = {}
    new_config["components"] = {}
    components = config.get("components", {})
    for component_name, component_config in components.items():
        new_config["components"][component_name] = {}
        new_config["components"][component_name]["description"] = component_config.get("description", "")
        # Here we start with the merged defaults, then apply the component config on top, so that any fields specified in the component config will override the defaults.
        new_config["components"][component_name]["component_properties"] = dict_merge(merged_defaults["component_properties"], component_config.get("component_properties"))
        new_config["components"][component_name]["deployment_config"] = dict_merge(merged_defaults["deployment_config"], component_config.get("deployment_config", {}))

        # Special case:
        # If the defaults specify alive_supervision for component, but the component config sets the type to anything other than "SUPERVISED", then we should not apply the
        # alive_supervision defaults to that component, since it doesn't make sense to have alive_supervision from the defaults.
        # TODO

    print(json.dumps(new_config, indent=4))

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
    with open(f"{output_dir}/hm_demo.json", "w") as hm_file:
        hm_config = {}
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
    preprocessed_config = preprocess_defaults(input_config)
    gen_health_monitor_config(args.output_dir, preprocessed_config)
    gen_launch_manager_config(args.output_dir, preprocessed_config)
    
    return 0

if __name__ == "__main__":
    exit(main())
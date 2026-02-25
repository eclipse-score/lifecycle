#!/usr/bin/env python3

import argparse
from copy import deepcopy
import json
import sys
from typing import Dict, Any

# TODO
json_schema = json.loads("""
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "s-core_launch_manager.schema.json",
  "title": "Configuration schema for the S-CORE Launch Manager",
  "type": "object",
  "$defs": {
    "component_properties": {
      "type": "object",
      "description": "Defines a reusable type that captures the essential development-time characteristics of a software component.",
      "properties": {
        "binary_name": {
          "type": "string",
          "description": "Specifies the relative path of the executable file inside the directory defined by 'deployment_config.bin_dir'. The final executable path will be resolved as '{bin_dir}/{binary_name}'. Example values include simple filenames (e.g., 'test_app1') or subdirectory paths (e.g., 'bin/test_app1')."
        },
        "application_profile": {
          "type": "object",
          "description": "Specifies the application profile that defines the runtime behavior and capabilities of this component.",
          "properties": {
            "application_type": {
              "type": "string",
              "enum": [
                "Native",
                "Reporting",
                "Reporting_And_Supervised",
                "State_Manager"
              ],
              "description": "Specifies the level of integration between the component and the Launch Manager. 'Native': no integration with Launch Manager. 'Reporting': uses Launch Manager lifecycle APIs. 'Reporting_And_Supervised': uses lifecycle APIs and sends alive notifications. 'State_Manager': uses lifecycle APIs, sends alive notifications, and has permission to change the active Run Target."
            },
            "is_self_terminating": {
              "type": "boolean",
              "description": "Indicates whether component is designed to terminate automatically once its planned tasks are completed (true), or remain running until explicitly requested to terminate by the Launch Manager (false)."
            },
            "alive_supervision": {
              "type": "object",
              "description": "Specifies the configuration parameters used for alive monitoring of the component.",
              "properties": {
                "reporting_cycle": {
                  "type": "number",
                  "exclusiveMinimum": 0,
                  "description": "Specifies the duration of the time interval used to verify that the component sends alive notifications, within the expected time frame."
                },
                "failed_cycles_tolerance": {
                  "type": "integer",
                  "minimum": 0,
                  "description": "Defines the maximum number of consecutive reporting cycle failures (see 'reporting_cycle'). Once the number of failed cycles goes above maximum number, Launch Manager will trigger configured recovery action."
                },
                "min_indications": {
                  "type": "integer",
                  "minimum": 0,
                  "description": "Specifies the minimum number of checkpoints that must be reported within each configured 'reporting_cycle'."
                },
                "max_indications": {
                  "type": "integer",
                  "minimum": 0,
                  "description": "Specifies the maximum number of checkpoints that may be reported within each configured 'reporting_cycle'."
                }
              },
              "required": [],
              "additionalProperties": false
            }
          },
          "required": [],
          "additionalProperties": false
        },
        "depends_on": {
          "type": "array",
          "description": "Names of components that this component depends on. Each dependency must be initialized and reach its ready state before this component can start.",
          "items": {
            "type": "string",
            "description": "Specifies the name of a component on which this component depends."
          }
        },
        "process_arguments": {
          "type": "array",
          "description": "Ordered list of command-line arguments passed to the component at startup.",
          "items": {
            "type": "string",
            "description": "Single command-line argument token as a UTF-8 string; order is preserved."
          }
        },
        "ready_condition": {
          "type": "object",
          "description": "Specifies the set of conditions that mark when the component completes its initializing state and enters the ready state.",
          "properties": {
            "process_state": {
              "type": "string",
              "enum": [
                "Running",
                "Terminated"
              ],
              "description": "Specifies the required state of the component's POSIX process. 'Running': the process has started and reached its running state. 'Terminated': the process has started, reached its running state, and then terminated successfully."
            }
          },
          "required": [],
          "additionalProperties": false
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "recovery_action": {
      "type": "object",
      "description": "Defines a reusable type that specifies which recovery actions should be executed when an error or failure occurs.",
      "properties": {
        "restart": {
          "type": "object",
          "description": "Specifies a recovery action that restarts the POSIX process associated with this component.",
          "properties": {
            "number_of_attempts": {
              "type": "integer",
              "minimum": 0,
              "description": "Specifies the maximum number of restart attempts before the Launch Manager concludes that recovery cannot succeed."
            },
            "delay_before_restart": {
              "type": "number",
              "minimum": 0,
              "description": "Specifies the delay duration that Launch Manager shall wait before initiating a restart attempt."
            }
          },
          "required": [],
          "additionalProperties": false
        },
        "switch_run_target": {
          "type": "object",
          "description": "Specifies a recovery action that switches to a Run Target. This can be a different Run Target or the same one to retry activation of the current Run Target.",
          "properties": {
            "run_target": {
              "type": "string",
              "description": "Specifies the name of the Run Target that Launch Manager should switch to."
            }
          },
          "required": [],
          "additionalProperties": false
        }
      },
      "oneOf": [
        {
          "required": [
            "restart"
          ]
        },
        {
          "required": [
            "switch_run_target"
          ]
        }
      ],
      "additionalProperties": false
    },
    "deployment_config": {
      "type": "object",
      "description": "Defines a reusable type that contains the configuration parameters that are specific to a particular deployment environment or system setup.",
      "properties": {
        "ready_timeout": {
          "type": "number",
          "exclusiveMinimum": 0,
          "description": "Specifies the maximum time allowed for the component to reach its ready state. The timeout is measured from when the component process is created until the ready conditions specified in 'component_properties.ready_condition' are met."
        },
        "shutdown_timeout": {
          "type": "number",
          "exclusiveMinimum": 0,
          "description": "Specifies the maximum allowed time for the component to terminate after it receives a SIGTERM signal from the Launch Manager. The timeout is measured from the moment the Launch Manager sends the SIGTERM signal, until the Operating System notifies the Launch Manager that the child process has terminated."
        },
        "environmental_variables": {
          "type": "object",
          "description": "Defines the set of environment variables that will be passed to the component at startup.",
          "additionalProperties": {
            "type": "string",
            "description": "Specifies the environment variable's value as a string. An empty string is allowed and represents an intentionally empty environment variable."
          }
        },
        "bin_dir": {
          "type": "string",
          "description": "Specifies the absolute filesystem path to the directory where component is installed."
        },
        "working_dir": {
          "type": "string",
          "description": "Specifies the directory that will be used as the working directory for the component during execution."
        },
        "ready_recovery_action": {
          "allOf": [
            {
              "$ref": "#/$defs/recovery_action"
            },
            {
              "properties": {
                "restart": true
              },
              "required": [
                "restart"
              ],
              "not": {
                "required": [
                  "switch_run_target"
                ]
              }
            }
          ],
          "description": "Specifies the recovery action to execute when the component fails to reach its ready state within the configured timeout."
        },
        "recovery_action": {
          "allOf": [
            {
              "$ref": "#/$defs/recovery_action"
            },
            {
              "properties": {
                "switch_run_target": true
              },
              "required": [
                "switch_run_target"
              ],
              "not": {
                "required": [
                  "restart"
                ]
              }
            }
          ],
          "description": "Specifies the recovery action to execute when the component malfunctions after reaching its ready state."
        },
        "sandbox": {
          "type": "object",
          "description": "Specifies the sandbox configuration parameters that isolate and constrain the component's runtime execution.",
          "properties": {
            "uid": {
              "type": "integer",
              "minimum": 0,
              "description": "Specifies the POSIX user ID (UID) under which this component executes."
            },
            "gid": {
              "type": "integer",
              "minimum": 0,
              "description": "Specifies the primary POSIX group ID (GID) under which this component executes."
            },
            "supplementary_group_ids": {
              "type": "array",
              "description": "Specifies the list of supplementary POSIX group IDs (GIDs) assigned to this component.",
              "items": {
                "type": "integer",
                "minimum": 0,
                "description": "Single supplementary POSIX group ID (GID)"
              }
            },
            "security_policy": {
              "type": "string",
              "description": "Specifies the security policy or confinement profile name (such as an SELinux or AppArmor profile) assigned to the component."
            },
            "scheduling_policy": {
              "type": "string",
              "description": "Specifies the scheduling policy applied to the component's initial thread. Supported values correspond to OS-defined policies (e.g., FIFO, RR, OTHER).",
              "anyOf": [
                {
                  "enum": [
                    "SCHED_FIFO",
                    "SCHED_RR",
                    "SCHED_OTHER"
                  ]
                },
                {
                  "type": "string"
                }
              ]
            },
            "scheduling_priority": {
              "type": "integer",
              "description": "Specifies the scheduling priority applied to the component's initial thread."
            },
            "max_memory_usage": {
              "type": "integer",
              "exclusiveMinimum": 0,
              "description": "Specifies the maximum amount of memory, in bytes, that the component is permitted to use during runtime."
            },
            "max_cpu_usage": {
              "type": "integer",
              "exclusiveMinimum": 0,
              "description": "Specifies the maximum CPU usage limit for the component, expressed as a percentage of total CPU capacity."
            }
          },
          "additionalProperties": false
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "run_target": {
      "type": "object",
      "description": "Defines a reusable type that specifies the configuration parameters for a Run Target.",
      "properties": {
        "description": {
          "type": "string",
          "description": "User-defined description of the configured Run Target."
        },
        "depends_on": {
          "type": "array",
          "description": "Names of components and Run Targets that must be activated when this Run Target is activated.",
          "items": {
            "type": "string",
            "description": "Name of a component or Run Target that this Run Target depends on."
          }
        },
        "transition_timeout": {
          "type": "number",
          "description": "Time limit for the Run Target transition. If this limit is exceeded, the transition is considered as failed.",
          "exclusiveMinimum": 0
        },
        "recovery_action": {
          "allOf": [
            {
              "$ref": "#/$defs/recovery_action"
            },
            {
              "properties": {
                "switch_run_target": true
              },
              "required": [
                "switch_run_target"
              ],
              "not": {
                "required": [
                  "restart"
                ]
              }
            }
          ],
          "description": "Specifies the recovery action to execute when a component assigned to this Run Target fails."
        }
      },
      "required": [
        "recovery_action"
      ],
      "additionalProperties": false
    },
    "alive_supervision": {
      "type": "object",
      "description": "Defines a reusable type that contains the configuration parameters for alive supervision",
      "properties": {
        "evaluation_cycle": {
          "type": "number",
          "exclusiveMinimum": 0,
          "description": "Specifies the length of the time window used to assess incoming alive supervision reports."
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "watchdog": {
      "type": "object",
      "description": "Defines a reusable type that contains the configuration parameters for the external watchdog.",
      "properties": {
        "device_file_path": {
          "type": "string",
          "description": "Path to the external watchdog device file (e.g., /dev/watchdog)."
        },
        "max_timeout": {
          "type": "number",
          "minimum": 0,
          "description": "Specifies the maximum timeout value that the Launch Manager will configure on the external watchdog during startup. The external watchdog uses this timeout as the deadline for receiving periodic alive reports from the Launch Manager."
        },
        "deactivate_on_shutdown": {
          "type": "boolean",
          "description": "Specifies whether the Launch Manager disables the external watchdog during shutdown. When set to true, the watchdog is deactivated; when false, it remains active."
        },
        "require_magic_close": {
          "type": "boolean",
          "description": "When true, the Launch Manager will perform a defined shutdown sequence to inform the external watchdog that the shutdown is intentional and to prevent a watchdog-initiated reset."
        }
      },
      "required": [],
      "additionalProperties": false
    }
  },
  "properties": {
    "schema_version": {
      "type": "integer",
      "description": "Specifies the schema version number that the Launch Manager uses to determine how to parse and validate this configuration file.",
      "enum": [
        1
      ]
    },
    "defaults": {
      "type": "object",
      "description": "Specifies default configuration values that components and Run Targets inherit unless they provide their own overriding values.",
      "properties": {
        "component_properties": {
          "description": "Specifies default component property values applied to all components unless overridden in individual component definitions.",
          "$ref": "#/$defs/component_properties"
        },
        "deployment_config": {
          "description": "Specifies default deployment configuration values applied to all components unless overridden in individual component definitions.",
          "$ref": "#/$defs/deployment_config"
        },
        "run_target": {
          "description": "Specifies default Run Target configuration values applied to all Run Targets unless overridden in individual Run Target definitions.",
          "$ref": "#/$defs/run_target"
        },
        "alive_supervision": {
          "description": "Specifies default alive supervision configuration values that are used unless a global 'alive_supervision' configuration is defined at the root level.",
          "$ref": "#/$defs/alive_supervision"
        },
        "watchdog": {
          "description": "Specifies default watchdog configuration values applied to all watchdogs unless overridden in individual watchdog definitions.",
          "$ref": "#/$defs/watchdog"
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "components": {
      "type": "object",
      "description": "Defines software components managed by the Launch Manager, where each property name is a unique component identifier and its value contains the component's configuration.",
      "patternProperties": {
        "^[a-zA-Z0-9_-]+$": {
          "type": "object",
          "description": "Specifies an individual component's configuration properties and deployment settings.",
          "properties": {
            "description": {
              "type": "string",
              "description": "A human-readable description of the component's purpose."
            },
            "component_properties": {
              "description": "Specifies component properties for this component; any properties not specified here are inherited from 'defaults.component_properties'.",
              "$ref": "#/$defs/component_properties"
            },
            "deployment_config": {
              "description": "Specifies deployment configuration for this component; any properties not specified here are inherited from 'defaults.deployment_config'.",
              "$ref": "#/$defs/deployment_config"
            }
          },
          "required": [],
          "additionalProperties": false
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "run_targets": {
      "type": "object",
      "description": "Defines Run Targets representing different operational modes of the system, where each property name is a unique Run Target identifier and its value contains the Run Target's configuration.",
      "patternProperties": {
        "^[a-zA-Z0-9_-]+$": {
          "$ref": "#/$defs/run_target"
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "initial_run_target": {
      "type": "string",
      "description": "Specifies the initial Run Target name that the Launch Manager activates during startup sequence. This name shall match a Run Target defined in 'run_targets'."
    },
    "fallback_run_target": {
      "type": "object",
      "description": "Defines the fallback Run Target configuration that the Launch Manager activates when all recovery attempts have been exhausted. This Run Target does not include a recovery_action property.",
      "properties": {
        "description": {
          "type": "string",
          "description": "A human-readable description of the fallback Run Target."
        },
        "depends_on": {
          "type": "array",
          "description": "Names of components and Run Targets that must be activated when this Run Target is activated.",
          "items": {
            "type": "string",
            "description": "Name of a component or Run Target that this Run Target depends on."
          }
        },
        "transition_timeout": {
          "type": "number",
          "description": "Time limit for the Run Target transition. If this limit is exceeded, the transition is considered as failed.",
          "exclusiveMinimum": 0
        }
      },
      "required": [],
      "additionalProperties": false
    },
    "alive_supervision": {
      "description": "Defines alive supervision configuration parameters used to monitor component health. This configuration overrides 'defaults.alive_supervision' if specified.",
      "$ref": "#/$defs/alive_supervision"
    },
    "watchdogs": {
      "type": "object",
      "description": "Defines external watchdog devices used by the Launch Manager, where each property name is a unique watchdog identifier and its value contains the watchdog's configuration.",
      "patternProperties": {
        "^[a-zA-Z0-9_-]+$": {
          "$ref": "#/$defs/watchdog"
        }
      },
      "required": [],
      "additionalProperties": false
    }
  },
  "description": "Specifies the structure and valid values for the Launch Manager configuration file, which defines managed components, run targets, and recovery behaviors.",
  "required": [
    "schema_version"
  ],
  "additionalProperties": false
}
""")

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
""")

# There are various dictionaries in the config where only a single entry is allowed.
# We do not want to merge the defaults with the user specified values for these dictionaries.
not_merging_dicts = ["ready_recovery_action", "recovery_action"]


def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load and parse a JSON file."""
    with open(file_path, "r") as file:
        return json.load(file)


def get_recovery_process_group_state(config):
    fallback = config.get("fallback_run_target", None)
    if fallback:
        return "MainPG/fallback_run_target"
    else:
        return "MainPG/Off"


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
    new_config["watchdogs"] = dict_merge(
        merged_defaults["watchdogs"], config.get("watchdogs", {})
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

            with open(f"{output_dir}/{component_name}.json", "w") as process_file:
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
    for watchdog_name, watchdog_config in config.get("watchdogs", {}).items():
        watchdog = {}
        watchdog["shortName"] = watchdog_name
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
                        f"Run target depends on unknown run target '{dependency_name}'."
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

    if "fallback_run_target" in config["run_targets"]:
        print(
            "Run target name fallback_run_target is reserved at the moment",
            file=sys.stderr,
        )
        exit(1)

    lm_config = {}
    lm_config["versionMajor"] = 7
    lm_config["versionMinor"] = 0
    lm_config["Process"] = []
    lm_config["ModeGroup"] = [
        {
            "identifier": "MainPG",
            "initialMode_name": config.get("initial_run_target", "Off"),
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


def schema_validation(json_input, schema):
    from jsonschema import validate, ValidationError

    try:
        validate(json_input, schema)
        print("Schema Validation successful")
        return True
    except ValidationError as err:
        print(err)
        return False


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
    args = parser.parse_args()

    input_config = load_json_file(args.filename)

    validation_successful = schema_validation(input_config, json_schema)
    if not validation_successful:
        exit(1)

    if args.validate:
        exit(0)

    preprocessed_config = preprocess_defaults(score_defaults, input_config)
    gen_health_monitor_config(args.output_dir, preprocessed_config)
    gen_launch_manager_config(args.output_dir, preprocessed_config)

    return 0


if __name__ == "__main__":
    exit(main())

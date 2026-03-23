#!/usr/bin/env python3
from scripts.config_mapping.lifecycle_config import preprocess_defaults
import json


def test_preprocessing_basic():
    """
    Basic smoketest for the preprocess_defaults function, to ensure that defaults are being applied and overridden correctly.
    """

    global_defaults = json.loads("""
    {
        "deployment_config": {
            "ready_timeout": 0.5,
            "shutdown_timeout": 0.5,
            "environmental_variables" : {
                "global_default1": "global_default_value1",
                "global_default2": "global_default_value2"
            },
            "sandbox": {
                "uid": 0,
                "supplementary_group_ids": [100]
            }
        },
        "component_properties": {
            "application_profile": {
                "application_type": "REPORTING",
                "is_self_terminating": false
            }
        },
        "alive_supervision": {
            "evaluation_cycle": 0.5
        },
        "watchdog": {}
    }""")

    config = json.loads("""{
        "defaults": {
            "deployment_config": {
                "shutdown_timeout": 1.0,
                "environmental_variables" : {
                    "global_default2": "config_default_overwritten_value2",
                    "config_default3": "config_default_value3",
                    "config_default4": "config_default_value4"
                },
                "recovery_action": {
                    "restart": {
                        "number_of_attempts": 1,
                        "delay_before_restart": 0.5
                    }
                }
            },
            "component_properties": {

            }
        },
        "components": {
            "test_comp": {
                "description": "Test component",
                "component_properties": {

                },
                "deployment_config": {
                    "environmental_variables": {
                        "config_default3": "config_overwritten_value3"
                    },
                    "sandbox": {
                        "uid": 0,
                        "gid": 1,
                        "supplementary_group_ids": [101]
                    },
                    "recovery_action": {
                        "switch_run_target": {
                            "run_target": "Off"
                        }
                    }
                }
            }
        },
        "run_targets": {},
        "alive_supervision": {
            "evaluation_cycle": 0.1
        },
        "watchdog": {
            "device_file_path": "/dev/watchdog",
            "max_timeout": 2,
            "deactivate_on_shutdown": true,
            "require_magic_close": false
        }
    }""")

    preprocessed_config = preprocess_defaults(global_defaults, config)

    expected_config = json.loads("""{
        "components": {
            "test_comp": {
                "description": "Test component",
                "component_properties": {
                    "application_profile": {
                        "application_type": "REPORTING",
                        "is_self_terminating": false
                    }
                },
                "deployment_config": {
                    "ready_timeout": 0.5,
                    "shutdown_timeout": 1.0,
                    "environmental_variables" : {
                        "global_default1": "global_default_value1",
                        "global_default2": "config_default_overwritten_value2",
                        "config_default3": "config_overwritten_value3",
                        "config_default4": "config_default_value4"
                    },
                    "sandbox": {
                        "uid": 0,
                        "gid":1,
                        "supplementary_group_ids": [101]
                    },
                    "recovery_action": {
                        "switch_run_target": {
                            "run_target": "Off"
                        }
                    }
                }
            }
        },
        "run_targets": {},
        "alive_supervision": {
            "evaluation_cycle": 0.1
        },
        "watchdog": {
            "device_file_path": "/dev/watchdog",
            "max_timeout": 2,
            "deactivate_on_shutdown": true,
            "require_magic_close": false
        }
    }""")

    print("Dumping preprocessed configuration:")
    print(json.dumps(preprocessed_config, indent=4))

    assert preprocessed_config == expected_config, (
        "Preprocessed config does not match expected config."
    )

#!/usr/bin/env python3
from scripts.config_mapping.lifecycle_config import preprocess_defaults, gen_config, score_defaults
import json
import os
import tempfile


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


def _make_preprocessed_config():
    """Helper that returns a fully-preprocessed config suitable for gen_config()."""
    input_config = {
        "schema_version": 1,
        "components": {
            "app_a": {
                "description": "First app",
                "component_properties": {
                    "binary_name": "app_a_bin",
                    "application_profile": {
                        "application_type": "Reporting_And_Supervised",
                        "is_self_terminating": False,
                        "alive_supervision": {
                            "reporting_cycle": 0.5,
                            "failed_cycles_tolerance": 2,
                            "min_indications": 1,
                            "max_indications": 3,
                        },
                    },
                    "depends_on": [],
                    "process_arguments": ["--flag"],
                    "ready_condition": {"process_state": "Running"},
                },
                "deployment_config": {
                    "bin_dir": "/opt/apps",
                },
            },
            "app_b": {
                "description": "Second app",
                "component_properties": {
                    "binary_name": "app_b_bin",
                    "application_profile": {
                        "application_type": "Native",
                        "is_self_terminating": True,
                    },
                    "depends_on": ["app_a"],
                    "process_arguments": [],
                    "ready_condition": {"process_state": "Terminated"},
                },
                "deployment_config": {
                    "bin_dir": "/opt/apps",
                },
            },
        },
        "run_targets": {
            "Startup": {
                "description": "Initial",
                "depends_on": ["app_a"],
                "transition_timeout": 5,
                "recovery_action": {
                    "switch_run_target": {"run_target": "fallback_run_target"}
                },
            },
        },
        "initial_run_target": "Startup",
        "fallback_run_target": {
            "description": "Safe state",
            "depends_on": [],
            "transition_timeout": 1.5,
            "recovery_action": {
                "switch_run_target": {"run_target": "fallback_run_target"}
            },
        },
        "alive_supervision": {"evaluation_cycle": 0.5},
        "watchdog": {
            "device_file_path": "/dev/watchdog",
            "max_timeout": 2,
            "deactivate_on_shutdown": True,
            "require_magic_close": False,
        },
    }
    config = preprocess_defaults(score_defaults, input_config)
    config["schema_version"] = input_config["schema_version"]
    config["initial_run_target"] = input_config["initial_run_target"]
    config["fallback_run_target"] = preprocess_defaults(
        score_defaults, input_config
    )["fallback_run_target"]
    return config


def _run_gen_config(config):
    """Run gen_config into a temp directory and return the parsed JSON output."""
    with tempfile.TemporaryDirectory() as tmpdir:
        gen_config(tmpdir, config)
        output_path = os.path.join(tmpdir, "launch_manager_config.json")
        with open(output_path) as f:
            return json.load(f)


def test_gen_config_produces_single_file():
    """gen_config writes exactly one file named launch_manager_config.json."""
    config = _make_preprocessed_config()
    with tempfile.TemporaryDirectory() as tmpdir:
        gen_config(tmpdir, config)
        files = os.listdir(tmpdir)
        assert files == ["launch_manager_config.json"], (
            f"Expected single output file, got: {files}"
        )


def test_gen_config_top_level_structure():
    """Output contains all required top-level keys matching new_lm_flatcfg.fbs."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    required_keys = {
        "schema_version",
        "components",
        "run_targets",
        "initial_run_target",
        "fallback_run_target",
        "alive_supervision",
        "watchdog",
    }
    assert required_keys.issubset(output.keys()), (
        f"Missing top-level keys: {required_keys - output.keys()}"
    )


def test_gen_config_components_are_list():
    """Components are emitted as a list (not a dict), each with a 'name' field."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    assert isinstance(output["components"], list)
    names = [c["name"] for c in output["components"]]
    assert "app_a" in names
    assert "app_b" in names


def test_gen_config_component_fields():
    """Each component has the expected nested structure."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    comp = next(c for c in output["components"] if c["name"] == "app_a")

    assert comp["description"] == "First app"
    assert comp["component_properties"]["application_profile"]["application_type"] == "Reporting_And_Supervised"
    assert comp["component_properties"]["application_profile"]["is_self_terminating"] is False
    assert comp["component_properties"]["application_profile"]["alive_supervision"]["reporting_cycle"] == 0.5
    assert comp["component_properties"]["process_arguments"] == ["--flag"]
    assert comp["component_properties"]["ready_condition"]["process_state"] == "Running"

    assert comp["deployment_config"]["ready_timeout"] == 0.5
    assert comp["deployment_config"]["shutdown_timeout"] == 0.5
    assert comp["deployment_config"]["executable_path"] == "/opt/apps/app_a_bin"
    assert "sandbox" in comp["deployment_config"]
    assert comp["deployment_config"]["sandbox"]["uid"] == 1000
    assert comp["deployment_config"]["sandbox"]["scheduling_policy"] == "OTHER"


def test_gen_config_run_targets_are_list():
    """Run targets are emitted as a list with a 'name' field."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    assert isinstance(output["run_targets"], list)
    rt = output["run_targets"][0]
    assert rt["name"] == "Startup"
    assert rt["transition_timeout"] == 5
    assert rt["recovery_action"]["run_target"] == "fallback_run_target"
    assert rt["depends_on"] == ["app_a"]


def test_gen_config_fallback_run_target():
    """Fallback run target is a flat object with transition_timeout."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    frt = output["fallback_run_target"]
    assert frt["transition_timeout"] == 1.5


def test_gen_config_alive_supervision():
    """Alive supervision section has evaluation_cycle in seconds."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    assert output["alive_supervision"]["evaluation_cycle"] == 0.5


def test_gen_config_watchdog():
    """Watchdog section contains all required fields."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    wd = output["watchdog"]
    assert wd["device_file_path"] == "/dev/watchdog"
    assert wd["max_timeout"] == 2
    assert wd["deactivate_on_shutdown"] is True
    assert wd["require_magic_close"] is False


def test_gen_config_no_watchdog_when_empty():
    """When watchdog config has no required fields, it is omitted from output."""
    config = _make_preprocessed_config()
    config["watchdog"] = {}
    output = _run_gen_config(config)

    assert "watchdog" not in output


def test_gen_config_env_vars_as_key_value_list():
    """Environmental variables are emitted as a list of {key, value} objects."""
    config = _make_preprocessed_config()
    config["components"]["app_a"]["deployment_config"]["environmental_variables"] = {
        "FOO": "bar",
        "BAZ": "qux",
    }
    output = _run_gen_config(config)

    comp = next(c for c in output["components"] if c["name"] == "app_a")
    env_vars = comp["deployment_config"]["environmental_variables"]
    assert isinstance(env_vars, list)
    keys = {ev["key"] for ev in env_vars}
    assert keys == {"FOO", "BAZ"}


def test_gen_config_native_app_no_alive_supervision():
    """Native application type does not include alive_supervision in output."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    native_comp = next(c for c in output["components"] if c["name"] == "app_b")
    assert "alive_supervision" not in native_comp["component_properties"]["application_profile"]


def test_gen_config_depends_on_preserved():
    """Component depends_on is preserved in the output."""
    config = _make_preprocessed_config()
    output = _run_gen_config(config)

    comp_b = next(c for c in output["components"] if c["name"] == "app_b")
    assert comp_b["component_properties"]["depends_on"] == ["app_a"]

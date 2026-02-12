# Motivation

We are introducing a new, simpler configuration file for the launch_manager.
To make use of the new configuration as early as possible, we are introducing a script to map the new configuration to the old configuration.
Once the source code of the launch_manager has been adapted to read in the new configuration file, the mapping script will become obsolete.

# Usage

Providing a json file using the new configuration format as input, the script will map the content to the old configuration file format and generate those files into the specified output_dir.

```
python3 lifecycle_config.py <new_configuration.json> -o <output_dir>
```

# Running Tests

You may want to use the virtual environment:

```bash
python3 -m venv myvenv
. myvenv/bin/activate
pip3 install -r requirements.txt
```

Execute all tests:

```bash
pytest tests.py
```

# Mapping Details

## Mapping of RunTargets to ProcessGroups

TODO

## Mapping of Components to Processes

TODO

### Mapping of Deployment Config to Startup Config

TODO

### Mapping of ReadyCondition to Execution Dependencies

TODO

## Mapping of Recovery Actions

## Known Limitations

TODO

* Transition timeout
* terminated ready condition

# Note for Developers

The `ConfigurationAdapter` serves as a temporary bridge that translates the new Config model (RunTargets, Components) into the
legacy API (ProcessGroups, ProcessGroupStates, OsProcess) consumed by ProcessGroupManager, Graph,
and related code. It exists only to decouple the config-format migration from the broader code migration and should be removed once ProcessGroupManager and its dependents are adapted to work directly with RunTarget/Component concepts.

Ensure that the following tests are successfull at every stage of the new configuration adaption:

```bash
bazel test --config=x86_64-linux --//config:use_new_configuration=True //score/launch_manager/...
bazel test --config=x86_64-linux --//config:use_new_configuration=True //tests/integration/...
bazel test --config=x86_64-linux --//config:use_new_configuration=True //examples/...
```

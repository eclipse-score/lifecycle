load("@score_lifecycle_pip//:requirements.bzl", "requirement")
load("@score_tooling//:defs.bzl", "score_py_pytest")

filegroup(
    name = "integration_test_files",
    srcs = glob(["**/*/*.json"]),
)

score_py_pytest(
    name = "lifecycle_config_tests",
    srcs = [
        "//scripts/config_mapping:integration_tests.py",
        "//scripts/config_mapping:unit_tests.py",
    ],
    args = [
    ],
    data = [
        ":integration_test_files",
        "//score/mw/launch_manager/configuration/config_schema:launch_manager.schema.json",
        "//scripts/config_mapping:lifecycle_config.py",
    ],
    tags = ["manual"],
    deps = [
        requirement("pytest"),
        requirement("jsonschema"),
    ],
)

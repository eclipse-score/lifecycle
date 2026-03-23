load("@score_lifecycle_pip//:requirements.bzl", "requirement")
load("@score_tooling//:defs.bzl", "score_py_pytest")

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    visibility = ["//visibility:public"],
    deps = [requirement("jsonschema")],
)

filegroup(
    name = "integration_test_files",
    srcs = glob(["tests/**/*/*.json"]),
)

score_py_pytest(
    name = "lifecycle_config_tests",
    srcs = [
        "integration_tests.py",
        "unit_tests.py",
    ],
    args = [
    ],
    data = [
        ":integration_test_files",
        ":lifecycle_config.py",
        "//src/launch_manager_daemon/config/config_schema:s-core_launch_manager.schema.json",
    ],
    tags = ["manual"],
    deps = [
        requirement("pytest"),
        requirement("jsonschema"),
    ],
)

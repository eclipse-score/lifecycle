load("@score_lifecycle_pip//:requirements.bzl", "requirement")

exports_files([
    "integration_tests.py",
    "unit_tests.py",
    "lifecycle_config.py",
])

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    visibility = ["//visibility:public"],
    deps = [requirement("jsonschema")],
)

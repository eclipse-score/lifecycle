
load("@score_lifecycle_pip//:requirements.bzl", "requirement")

# Example Usage
# load("//scripts/config_mapping:config.bzl", "gen_lifecycle_config")
# exports_files(["lm_config.json"])
# gen_lifecycle_config(
#     name ="example_config_gen",
#     config="//scripts/config_mapping:lm_config.json")

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    deps = [requirement("jsonschema")],
    visibility = ["//visibility:public"],
)

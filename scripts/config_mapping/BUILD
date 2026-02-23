
load("//scripts/config_mapping:config.bzl", "gen_lifecycle_config")

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    visibility = ["//visibility:public"],
)

# Example Usage
exports_files(["lm_config.json"])
gen_lifecycle_config(
    name ="example_config_gen",
    config="//scripts/config_mapping:lm_config.json")


load("//scripts/config_mapping:config.bzl", "lifecycle_config")
load("//scripts/config_mapping:config.bzl", "lifecycle_config_action")

exports_files(["lm_config.json"])

#sh_binary(
#    name = "lifecycle_config",
#    srcs = ["lifecycle_config_wrapper.sh"],
#    data = [
#        "lifecycle_config.py", 
#        "requirements.txt",
#    ],
#    visibility = ["//visibility:public"],
#)


py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
)

#lifecycle_config(config_path="//scripts/config_mapping:lm_config.json", 
#    output_files=["hm_demo.json", "lm_demo.json", "hmcore.json", "someip-daemon.json",
#                  "state_manager.json", "test_app1.json"])
#

lifecycle_config_action(
    name ="lifecycle_config_action",
    config="//scripts/config_mapping:lm_config.json"
)

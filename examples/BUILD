load("//scripts/config_mapping:config.bzl", "gen_lifecycle_config")
gen_lifecycle_config(
    name ="example_config",
    config="//examples/config:lifecycle_demo_config")

filegroup(
    name = "example_apps",
    srcs = [
        "//examples/control_application:control_daemon",
        "//examples/control_application:lmcontrol",
        "//examples/cpp_supervised_app:cpp_supervised_app",
        "//examples/rust_supervised_app:rust_supervised_app",
        "//examples:example_config"
    ],
)

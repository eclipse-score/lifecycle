load("@pip_score_venv_test//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

py_library(
    name = "fixtures",
    srcs = [
        "target.py",
        "ssh.py",
        "control_interface.py",
        "linux_interface.py",
        "__init__.py",
    ],
    visibility = ["//tests:__subpackages__"],
    deps = [
        requirement("pytest"),
        "@score_itf//itf/core/qemu",
        "@score_itf//itf/core/com:ssh",
        "@score_itf//itf/core/utils/process",
        ":conf"
    ],
    tags = [
        "local",
        "integration"
    ]
)


py_library(
    name="conf",
    srcs = [ "conftest.py" ],
    visibility = ["//tests:__subpackages__"],
)

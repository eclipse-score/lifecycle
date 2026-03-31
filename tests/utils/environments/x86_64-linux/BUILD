load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")

oci_image(
    name = "image",
    base = "@debian-test-runtime",
)

oci_load(
    name = "x86_64-linux",
    image = ":image",
    repo_tags = ["score_itf_examples:latest"],
    visibility = [
        "//tests:__subpackages__",
    ],
)

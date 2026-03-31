load("//tests/utils/bazel:constants.bzl", "SCORE_TEST_INSTALL_PREFIX")

def _package_test_binaries_impl(ctx):
    """Packages test binaries into a given structure.
    """
    output_files = []
    prefix = ctx.attr.install_prefix.lstrip("/")

    for target, relative_location in ctx.attr.binaries.items():
        # it's possible a target is composed of multiple files so link all
        for file in target.files.to_list():
            if file.is_directory:
                dir_name = relative_location if relative_location else file.basename
                output_file = ctx.actions.declare_directory("{prefix}/tests/{test_name}/{dir_name}".format(
                    prefix = prefix,
                    test_name = ctx.attr.test_name,
                    dir_name = dir_name,
                ))
            else:
                output_file = ctx.actions.declare_file("{prefix}/tests/{test_name}/{relative_location}/{proc}".format(
                    prefix = prefix,
                    test_name = ctx.attr.test_name,
                    relative_location = relative_location,
                    proc = file.basename,
                ))
            output_files.append(output_file)

            ctx.actions.symlink(
                output = output_file,
                target_file = file,
            )

    return [DefaultInfo(files = depset(output_files))]

package_test_binaries = rule(
    doc =
        """Packages binaries into a given structure.
    The binaries are symlinked.

    @details
    The file structure of the pacakge will be:
    `<current build dir>/<install_prefix>/tests/<test name>/<given path>`

    """,
    implementation = _package_test_binaries_impl,
    attrs = {
        "test_name": attr.string(
            mandatory = True,
            doc = "Name of the test that the binaries will belong to",
        ),
        "binaries": attr.label_keyed_string_dict(
            mandatory = True,
            allow_files = True,
            doc = "Dictionary mapping targets (need to be files) to a location",
        ),
        "install_prefix": attr.string(
            default = SCORE_TEST_INSTALL_PREFIX,
            doc = "Base installation prefix for the packaged binaries. Defaults to SCORE_TEST_INSTALL_PREFIX.",
        ),
    },
)

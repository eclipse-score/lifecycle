def _package_test_binaries_impl(ctx):
    """Packages test binaries into a given structure.
    """
    output_files = []

    for target, relative_location in ctx.attr.binaries.items():
        # it's possible a target is composed of multiple files so link all
        for file in target.files.to_list():
            output_file = ctx.actions.declare_file("opt/score/tests/{test_name}/{relative_location}/{proc}".format(
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
    `<current build dir>/opt/score/<test name>/<given path>`

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
    },
)

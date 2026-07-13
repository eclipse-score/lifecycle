load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("@score_baselibs//score/flatbuffers/bazel:tools.bzl", "serialize_buffer")

def _lm_config_splitter_impl(ctx):
    """Run a script to convert from the new config structure to the old, this 
    creates 3 seperate configs.
    """
    script = ctx.executable.script
    config = ctx.file.config
    schema = ctx.file.schema

    ctx.actions.run(
        inputs = [config, schema],
        outputs = [ctx.outputs.lm_json, ctx.outputs.hm_json, ctx.outputs.hmcore_json],
        tools = [script],
        mnemonic = "LifecycleJsonConfigGeneration",
        executable = script,
        progress_message = "generating Launch Manager config from {}".format(config.short_path),
        arguments = [
            config.path,
            "--schema",
            schema.path,
            "-o",
            ctx.outputs.lm_json.dirname,
        ],
    )

lm_config_splitter = rule(
    implementation = _lm_config_splitter_impl,
    doc = "Splits the new configuration format int othe old 3 formats.",
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert.",
        ),
        "schema": attr.label(
            default = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json"),
            allow_single_file = [".json"],
            doc = "Json schema file to validate the input json against",
        ),
        "script": attr.label(
            default = Label("//scripts/config_mapping:lifecycle_config"),
            executable = True,
            cfg = "exec",
            doc = "Python script to execute",
        ),
    },
    outputs = {
        "lm_json": "%{name}/lm_demo.json",
        "hm_json": "%{name}/hm_demo.json",
        "hmcore_json": "%{name}/hmcore.json",
    },
)

def _lm_new_config_gen_impl(ctx):
    """Run lifecycle_config.py --new-format to produce a single unified config."""
    script = ctx.executable.script
    config = ctx.file.config
    schema = ctx.file.schema
    base = config.basename
    dot_index = base.rfind(".")
    stem = base if dot_index == -1 else base[:dot_index]
    output_name = "{}_gen.json".format(stem)
    out_file = ctx.actions.declare_file("{}/{}".format(ctx.label.name, output_name))

    ctx.actions.run(
        inputs = [config, schema],
        outputs = [out_file],
        tools = [script],
        mnemonic = "LifecycleNewFormatJsonGeneration",
        executable = script,
        progress_message = "generating new-format Launch Manager config from {}".format(config.short_path),
        arguments = [
            config.path,
            "--schema",
            schema.path,
            "-o",
            out_file.dirname,
            "--new-format",
        ],
    )

    return [DefaultInfo(files = depset([out_file]))]

lm_new_config_generator = rule(
    implementation = _lm_new_config_gen_impl,
    doc = "Generates a single unified configuration file in the new flatbuffer format.",
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert.",
        ),
        "schema": attr.label(
            default = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json"),
            allow_single_file = [".json"],
            doc = "Json schema file to validate the input json against",
        ),
        "script": attr.label(
            default = Label("//scripts/config_mapping:lifecycle_config"),
            executable = True,
            cfg = "exec",
            doc = "Python script to execute",
        ),
    },
)

def _lm_config_combiner_impl(ctx):
    """Declare a single directory that all serialized buffers get copied into.
    Consumers receive this one directory instead of individual files.
    """
    etc_dir = ctx.actions.declare_directory(ctx.attr.dir_name)

    srcs = ctx.files.srcs

    args = ctx.actions.args()
    args.add(etc_dir.path)
    args.add_all([f.path for f in srcs])

    ctx.actions.run_shell(
        inputs = srcs,
        outputs = [etc_dir],
        arguments = [args],
        command = 'dest="$1"; shift; mkdir -p "$dest"; for f in "$@"; do cp "$f" "$dest/"; done',
        mnemonic = "LmConfigCombine",
        progress_message = "Combining launch manager configs into {}".format(etc_dir.short_path),
    )

    return [DefaultInfo(files = depset([etc_dir]))]

lm_config_combiner = rule(
    implementation = _lm_config_combiner_impl,
    doc = "Combines the generated .bin files into a single etc directory.",
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
            doc = "List of .bin files to copy into the output directory.",
        ),
        "dir_name": attr.string(
            default = "etc",
            doc = "Name of the directory to place the combined config files into.",
        ),
    },
)

def launch_manager_config(
        name,
        config,
        schema = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json"),
        script = Label("//scripts/config_mapping:lifecycle_config"),
        flatbuffer_out_dir = "flatbuffer_out",
        lm_schema = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:lm_flatcfg.fbs"),
        new_lm_schema = Label("//score/launch_manager/src/daemon/src/configuration:new_lm_flatcfg_fbs"),
        hm_schema = Label("//score/launch_manager/src/daemon/src/alive_monitor/config:hm_flatcfg.fbs"),
        hmcore_schema = Label("//score/launch_manager/src/daemon/src/alive_monitor/config:hmcore_flatcfg.fbs"),
        **kwargs):
    split_name = name + "_split"

    # Old format path (3 separate flatbuffer files)
    # note that the splitting and combining is a workaround as we have to return
    # the etc directory where all the .bin files are for backwards compatibility.
    lm_config_splitter(
        name = split_name,
        config = config,
        schema = schema,
        script = script,
    )

    json_prefix = ":" + split_name + "/"

    old_format_buffers = [
        ("_lm_bin", "lm_demo", lm_schema),
        ("_hm_bin", "hm_demo", hm_schema),
        ("_hmcore_bin", "hmcore", hmcore_schema),
    ]

    for suffix, stem, schema_file in old_format_buffers:
        serialize_buffer(
            name = name + suffix,
            data = json_prefix + stem + ".json",
            schema = schema_file,
            output = name + "_serialized/" + stem + ".bin",
            **kwargs
        )

    # New format path (single unified flatbuffer file)
    new_gen_name = name + "_new_gen"
    lm_new_config_generator(
        name = new_gen_name,
        config = config,
        schema = schema,
        script = script,
    )

    config_basename = Label(config).name
    dot_index = config_basename.rfind(".")
    config_stem = config_basename[:dot_index] if dot_index != -1 else config_basename

    serialize_buffer(
        name = name + "_new_lm_bin",
        data = ":" + new_gen_name,
        schema = new_lm_schema,
        output = name + "_serialized_new/" + config_stem + ".bin",
        **kwargs
    )

    # Single combiner whose sources are selected by --//config:use_new_configuration
    # Always outputs to `flatbuffer_out_dir` (e.g. "etc") so the launch_manager
    # binary can find its config at "etc/<file>.bin" regardless of format.
    lm_config_combiner(
        name = name,
        srcs = select({
            Label("//config:lm_use_new_configuration"): [":" + name + "_new_lm_bin"],
            "//conditions:default": [
                ":" + name + "_lm_bin",
                ":" + name + "_hm_bin",
                ":" + name + "_hmcore_bin",
            ],
        }),
        dir_name = flatbuffer_out_dir,
    )

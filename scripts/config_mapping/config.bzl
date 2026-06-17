def _launch_manager_config_impl(ctx):
    config = ctx.file.config
    schema = ctx.file.schema
    script = ctx.executable.script
    json_out_dir = ctx.attr.json_out_dir

    gen_dir_json = ctx.actions.declare_directory(json_out_dir)
    ctx.actions.run(
        inputs = [config, schema],
        outputs = [gen_dir_json],
        tools = [script],
        mnemonic = "LifecycleJsonConfigGeneration",
        executable = script,
        progress_message = "generating Launch Manager config from {}".format(config.short_path),
        arguments = [
            config.path,
            "--schema",
            schema.path,
            "-o",
            gen_dir_json.path,
        ],
    )

    flatbuffer_out_dir = ctx.attr.flatbuffer_out_dir
    flatc = ctx.executable.flatc
    lm_schema = ctx.file.lm_schema

    gen_dir_flatbuffer = ctx.actions.declare_directory(flatbuffer_out_dir)
    ctx.actions.run(
        inputs = [gen_dir_json, lm_schema],
        outputs = [gen_dir_flatbuffer],
        tools = [flatc],
        executable = flatc,
        mnemonic = "LaunchManagerFlatbufferConfigGeneration",
        progress_message = "compiling Launch Manager config in {} to flatbuffer in {}".format(gen_dir_json.short_path, gen_dir_flatbuffer.short_path),
        arguments = [
            "-b",
            "-o",
            gen_dir_flatbuffer.path,
            lm_schema.path,
            gen_dir_json.path + "/launch_manager_config.json",
        ],
    )

    rf = ctx.runfiles(
        files = [gen_dir_flatbuffer],
        root_symlinks = {
            ("_main/" + ctx.attr.flatbuffer_out_dir): gen_dir_flatbuffer,
        },
    )

    return DefaultInfo(files = depset([gen_dir_flatbuffer]), runfiles = rf)

launch_manager_config = rule(
    implementation = _launch_manager_config_impl,
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert. Note that the binary file will have the same name as the json (minus the suffix)",
        ),
        "schema": attr.label(
            default = Label("//score/launch_manager/daemon/src/configuration/config_schema:launch_manager.schema.json"),
            allow_single_file = [".json"],
            doc = "Json schema file to validate the input json against",
        ),
        "script": attr.label(
            default = Label("//scripts/config_mapping:lifecycle_config"),
            executable = True,
            cfg = "exec",
            doc = "Python script to execute",
        ),
        "json_out_dir": attr.string(
            default = "json_out",
            doc = "Directory to copy the generated file to. Do not include a trailing '/'",
        ),
        "flatbuffer_out_dir": attr.string(
            default = "flatbuffer_out",
            doc = "Directory to copy the generated file to. Do not include a trailing '/'",
        ),
        "flatc": attr.label(
            default = Label("@flatbuffers//:flatc"),
            executable = True,
            cfg = "exec",
            doc = "Reference to the flatc binary",
        ),
        "lm_schema": attr.label(
            allow_single_file = [".fbs"],
            default = Label("//score/launch_manager/daemon/src/configuration:new_lm_flatcfg_fbs"),
            doc = "Launch Manager fbs file to use",
        ),
    },
)

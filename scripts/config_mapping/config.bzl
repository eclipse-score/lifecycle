load("@score_baselibs//score/flatbuffers/bazel:tools.bzl", "serialize_buffer")

def _lm_config_splitter_impl(ctx):
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
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert. Note that the binary file will have the same name as the json (minus the suffix)",
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

def launch_manager_config(
        name,
        config,
        schema = Label("//score/launch_manager/daemon/src/configuration/config_schema:launch_manager.schema.json"),
        script = Label("//scripts/config_mapping:lifecycle_config"),
        flatbuffer_out_dir = "flatbuffer_out",
        lm_schema = Label("//score/launch_manager/daemon/src/configuration/config_schema:lm_flatcfg.fbs"),
        hm_schema = Label("//score/launch_manager/daemon/src/alive_monitor/config:hm_flatcfg.fbs"),
        hmcore_schema = Label("//score/launch_manager/daemon/src/alive_monitor/config:hmcore_flatcfg.fbs"),
        **kwargs):
    split_name = name + "_split"

    lm_config_splitter(
        name = split_name,
        config = config,
        schema = schema,
        script = script,
    )

    json_prefix = ":" + split_name + "/"

    buffers = [
        ("_lm_bin", "lm_demo", lm_schema),
        ("_hm_bin", "hm_demo", hm_schema),
        ("_hmcore_bin", "hmcore", hmcore_schema),
    ]

    for suffix, stem, schema_file in buffers:
        serialize_buffer(
            name = name + suffix,
            data = json_prefix + stem + ".json",
            schema = schema_file,
            output = flatbuffer_out_dir + "/" + stem + ".bin",
            **kwargs
        )

    native.filegroup(
        name = name,
        srcs = [
            ":" + name + "_lm_bin",
            ":" + name + "_hm_bin",
            ":" + name + "_hmcore_bin",
        ],
        **kwargs
    )

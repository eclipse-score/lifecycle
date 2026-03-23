def _launch_manager_config_impl(ctx):
    config = ctx.file.config
    schema = ctx.file.schema
    script = ctx.executable.script
    json_out_dir = ctx.attr.json_out_dir

    # Run the mapping script to generate the json files in the old configuration format
    # We need to declare an output directory, because we do not know upfront the name of the generated files nor the number of files.
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
    hm_schema = ctx.file.hm_schema
    hmcore_schema = ctx.file.hmcore_schema

    # We compile each of them via flatbuffer.
    # Based on the name of each generated file, we select the corresponding schema.
    gen_dir_flatbuffer = ctx.actions.declare_directory(flatbuffer_out_dir)
    ctx.actions.run_shell(
        inputs = [gen_dir_json, lm_schema, hm_schema, hmcore_schema],
        outputs = [gen_dir_flatbuffer],
        tools = [flatc],
        command = """
            mkdir -p {gen_dir_flatbuffer}
            # Process each file from generated directory
            for file in {gen_dir_json}/*; do
                if [ -f "$file" ]; then
                    filename=$(basename "$file")

                    if [[ "$filename" == "lm_"* ]]; then
                        schema={lm_schema}
                    elif [[ "$filename" == "hmcore"* ]]; then
                        schema={hmcore_schema}
                    elif [[ "$filename" == "hm_"* ]]; then
                        schema={hm_schema}
                    elif [[ "$filename" == "hmproc_"* ]]; then
                        schema={hm_schema}
                    else
                        echo "Unknown file type for $filename, skipping."
                        continue
                    fi

                    # Process with flatc
                    {flatc} -b -o {gen_dir_flatbuffer} "$schema" "$file"
                fi
            done
        """.format(
            gen_dir_flatbuffer = gen_dir_flatbuffer.path,
            gen_dir_json = gen_dir_json.path,
            lm_schema = lm_schema.path,
            hmcore_schema = hmcore_schema.path,
            hm_schema = hm_schema.path,
            flatc = flatc.path,
        ),
        arguments = [],
        mnemonic = "LaunchManagerFlatbufferConfigGeneration",
        progress_message = "compiling generated Launch Manager configs in {} to flatbuffer files in {}".format(gen_dir_json.short_path, gen_dir_flatbuffer.short_path),
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
            default = Label("//src/launch_manager_daemon/config/config_schema:s-core_launch_manager.schema.json"),
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
            default = Label("//src/launch_manager_daemon:lm_flatcfg_fbs"),
            doc = "Launch Manager fbs file to use",
        ),
        "hm_schema": attr.label(
            allow_single_file = [".fbs"],
            default = Label("//src/launch_manager_daemon/health_monitor_lib:hm_flatcfg_fbs"),
            doc = "HealthMonitor fbs file to use",
        ),
        "hmcore_schema": attr.label(
            allow_single_file = [".fbs"],
            default = Label("//src/launch_manager_daemon/health_monitor_lib:hmcore_flatcfg_fbs"),
            doc = "HealthMonitor core fbs file to use",
        ),
    },
)

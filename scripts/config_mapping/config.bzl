#load("//scripts/config_mapping/flatbuffers_rules.bzl", "flatbuffer_json_to_bin")

def lifecycle_config(config_path, output_files=[]):

    native.genrule(
        name = "gen_lifecycle_config",
        srcs = [config_path, "//src/launch_manager_daemon/config:s-core_launch_manager.schema.json"],
        outs = output_files,
        cmd = """
            python3 $(location //scripts/config_mapping:lifecycle_config.py) $(location {input_json}) \
                --schema $(location //src/launch_manager_daemon/config:s-core_launch_manager.schema.json) -o $(@D)
        """.format(input_json=config_path),

        tools = ["//scripts/config_mapping:lifecycle_config.py"],
        visibility = ["//visibility:public"],
    )

#def _flatbuffer_json_to_bin_impl_tmp(ctx):
#    flatc = ctx.executable.flatc
#    json = ctx.file.json
#    schema = ctx.file.schema
#
#    # flatc will name the file the same as the json (can't be changed)
#    out_name = json.basename[:-len(".json")] + ".bin"
#    out = ctx.actions.declare_file(out_name, sibling = json)
#
#    # flatc args ---------------------------------
#    flatc_args = [
#        "-b",
#        "-o",
#        out.dirname,
#    ]
#
#    for inc in ctx.attr.includes:
#        flatc_args.extend(["-I", inc.path])
#
#    if ctx.attr.strict_json:
#        flatc_args.append("--strict-json")
#
#    flatc_args.extend([schema.path, json.path])
#    # --------------------------------------------
#
#    ctx.actions.run(
#        inputs = [json, schema] + list(ctx.files.includes),
#        outputs = [out],
#        executable = flatc,
#        arguments = flatc_args,
#        progress_message = "flatc generation {}".format(json.short_path),
#        mnemonic = "FlatcGeneration",
#    )
#
#    rf = ctx.runfiles(
#        files = [out],
#        root_symlinks = {
#            ("_main/" + ctx.attr.out_dir + "/" + out_name): out,
#        },
#    )

def _lifecycle_config_impl(ctx):
    config = ctx.file.config
    schema = ctx.file.schema
    script = ctx.executable.script
    json_out_dir = ctx.attr.json_out_dir

    # Get Python runtime
    python_runtime = ctx.toolchains["@rules_python//python:toolchain_type"].py3_runtime
    python_exe = python_runtime.interpreter

    # First run_shell - creates directory with files inside
    gen_dir_json = ctx.actions.declare_directory(json_out_dir)
    ctx.actions.run_shell(
        inputs = [config, schema],
        outputs = [gen_dir_json],
        tools = [script, python_exe],
        command = """
            export PYTHON3={}
            export PATH="$(dirname {}):$PATH"
            {} {} --schema {} -o {}
        """.format(python_exe.path, python_exe.path, script.path, config.path, schema.path, gen_dir_json.path),
        arguments = []
    )

    flatbuffer_out_dir = ctx.attr.flatbuffer_out_dir
    flatc = ctx.executable.flatc
    lm_schema = ctx.file.lm_schema
    hm_schema = ctx.file.hm_schema
    hmcore_schema = ctx.file.hmcore_schema
    
    # Second run_shell - processes the files from the generated directory
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
            gen_dir_flatbuffer=gen_dir_flatbuffer.path,
            gen_dir_json=gen_dir_json.path,
            lm_schema=lm_schema.path,
            hmcore_schema=hmcore_schema.path,
            hm_schema=hm_schema.path,
            flatc=flatc.path
        ),
        arguments = []
    )
    
    return DefaultInfo(files = depset([gen_dir_json, gen_dir_flatbuffer]))


lifecycle_config_action = rule(
    implementation = _lifecycle_config_impl,
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert. Note that the binary file will have the same name as the json (minus the suffix)",
        ),
        "schema": attr.label(
            default=Label("//src/launch_manager_daemon/config:s-core_launch_manager.schema.json"),
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
            default=Label("//src/launch_manager_daemon/health_monitor_lib:hm_flatcfg_fbs"),
            doc = "HealthMonitor fbs file to use",
        ),
        "hmcore_schema": attr.label(
            allow_single_file = [".fbs"],
            default=Label("//src/launch_manager_daemon/health_monitor_lib:hmcore_flatcfg_fbs"),
            doc = "HealthMonitor core fbs file to use",
        )
    },
    toolchains = ["@rules_python//python:toolchain_type"],
)


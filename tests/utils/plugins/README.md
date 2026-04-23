# LocalTarget

An ITF plugin to allow for testing locally.
This implements the `itf.core.target.Target` & `itf.core.process.AsyncProcess`.

This will deploy the files into the directory `/tmp` on your host machine.
So before running please create those directories with permissions so that your
user can write to those directories.
Note that bazel mounts the `/tmp` directory inside of the sandbox so you will
not see the files inside the `/tmp` directory unless you use `--sandbox_add_mount_pair=/tmp`.

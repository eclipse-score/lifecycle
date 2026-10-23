bazel run --config=x86_64-linux \
            --sandbox_writable_path=/var/tmp \
            //quality/static_analysis:codeql_lint -- \
            --output-dir /tmp/codeql-results/linux \
            --output-prefix codeql-nightly \
            --report cert-cpp-l1 cert-c-l1 cpp-code-scanning code-complexity \
            --target $(bazel query 'kind("cc_library|cc_binary", //score/...)')

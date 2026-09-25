# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
import argparse
import os
import tempfile
import json
import subprocess
import datetime
import shutil
import re
import urllib.parse


TMP_PATH_FOR_DATABASES = "/var/tmp/codeql_databases"
CODING_STANDARDS_CONFIG_RELATIVE_PATH = "quality/static_analysis/coding-standards.yaml"


# Default query suite (relative to the MISRA C++ pack root) run by the analysis.
# Forward-slash relative path, used both to locate the suite on disk and as the
# suite selector in the `<pack>@<version>:<suite>` query specifier.
MISRA_DEFAULT_SUITE_NAME = "codeql-suites/misra-cpp-default.qls"
CERT_CPP_L1_SUITE_NAME = "codeql-suites/cert-cpp-l1.qls"
CERT_C_L1_SUITE_NAME = "codeql-suites/cert-c-l1.qls"

# The standard CodeQL C++ query pack and the alert-suppression query it ships
# with are bundled inside @codeql_bundle's own qlpacks/ directory (part of the
# CLI distribution itself), so they resolve without any --search-path entry.
CPP_CODE_SCANNING_SPEC = "codeql/cpp-queries:codeql-suites/cpp-code-scanning.qls"
ALERT_SUPPRESSION_SPEC = "codeql/cpp-queries:AlertSuppression.ql"

# Local, uncompiled query pack (ported from vsps_quality_packages//tools/codeql/
# code-complexity) providing file/function size and complexity metrics. Lives
# directly in this repo's source tree, so it is resolved relative to
# source_root rather than through Bazel runfiles.
COMPLEXITY_PACK_RELATIVE_DIR = "third_party/codeql/code_complexity"
COMPLEXITY_SUITE_SPEC = "code-complexity-queries:suites/thresholds.qls"

# Runfiles path of the vendored pre-compiled MISRA C++ query pack's manifest,
# used to anchor the pack root. Provided by the @codeql_coding_standards_compiled
# repository (see third_party/codeql/codeql_release_pack.bzl).
COMPILED_PACK_RUNFILE = "codeql_coding_standards_compiled/pack/qlpack.yml"

# Same idea for the CERT C++ and CERT C pre-compiled packs, both extracted
# from the same coding-standards-codeql-packs.zip release asset (see
# @codeql_coding_standards_cert_cpp_compiled / @codeql_coding_standards_cert_c_compiled
# in MODULE.bazel).
CERT_CPP_COMPILED_PACK_RUNFILE = "codeql_coding_standards_cert_cpp_compiled/pack/qlpack.yml"
CERT_C_COMPILED_PACK_RUNFILE = "codeql_coding_standards_cert_c_compiled/pack/qlpack.yml"

# Named checks selectable with --report (one or more, space-separated). When
# --report is omitted, only the MISRA C++ default suite is run (the historical,
# single-standard behavior). Each value maps to one query pack/suite in
# _build_report_analysis_spec(); the AlertSuppression helper query is always
# appended so `// codeql[...]` in-source suppressions keep working regardless of
# the selected checks.
REPORT_CHOICES = [
    "misra-default",
    "cert-cpp-l1",
    "cert-c-l1",
    "cpp-code-scanning",
    "code-complexity",
]
DEFAULT_REPORTS = ["misra-default"]


def _find_coding_standards_root():
    """Locate the vendored codeql-coding-standards repo root (the dir containing cpp/).

    The codeql_coding_standards repo's cpp/** sources are declared as a `data`
    dependency of @codeql_coding_standards//:analysis_report, which is in turn a
    `data` dependency of this py_binary, so Bazel places them in our runfiles
    tree. Only used for the `--query-spec` override, which analyzes a query from
    these sources rather than the pre-compiled release pack.
    """
    from python.runfiles import Runfiles

    runfiles = Runfiles.Create()
    anchor = runfiles.Rlocation("codeql_coding_standards/cpp/common/src/qlpack.yml")
    if not anchor or not os.path.exists(anchor):
        raise RuntimeError("Unable to locate CodeQL coding standards repo root")
    # anchor = <repo_root>/cpp/common/src/qlpack.yml -> go up four levels.
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(anchor))))


def _find_release_pack_root(runfile_path, pack_repo_label, required_suite=None):
    """Locate a pre-compiled coding-standards query pack root from runfiles.

    Generalizes over the MISRA C++, CERT C++ and CERT C pre-compiled packs,
    all vendored the same way by a @codeql_release_pack repository (see
    third_party/codeql/codeql_release_pack.bzl): a pre-compiled pack published
    with the codeql-coding-standards release, containing the compiled queries
    (`.qlx`), the default suites and all library dependencies bundled under
    `.codeql/libraries/`. Analyzing against such a pack (referenced by its
    `<name>@<version>:<suite>` specifier and made discoverable via
    --search-path) runs exactly the pinned ruleset without recompiling the
    queries and without downloading anything from the registry.

    These vendored packs are the ONLY supported query source for their
    respective standards: if one cannot be located we raise an error instead
    of silently falling back to any other pack (e.g. a registry download or a
    runtime-compiled pack), so that the analysis always runs exactly the
    pinned, hermetic ruleset.

    Returns the pack root directory (the one containing qlpack.yml).
    """
    from python.runfiles import Runfiles

    runfiles = Runfiles.Create()
    anchor = runfiles.Rlocation(runfile_path)
    if not anchor or not os.path.exists(anchor):
        raise RuntimeError(
            f"Vendored pre-compiled query pack not found (expected runfile "
            f"'{runfile_path}'). Ensure the {pack_repo_label}//:pack dependency "
            "is in this target's `data`. Refusing to fall back to any other "
            "query source."
        )
    # anchor = <pack_root>/qlpack.yml
    # Resolve symlinks: Bazel runfiles are a symlink farm where individual files
    # are symlinked into the runfiles tree.  CodeQL's data-extension glob
    # (qlpack.yml `dataExtensions:`) does NOT follow symlinks when matching
    # .model.yml files, so using the runfiles path causes all extensible
    # predicates (allocationFunctionModel, throwingFunctionModel, etc.) to come
    # up as undefined.  realpath() jumps from the runfiles symlink to the actual
    # Bazel external-repository directory, where the model files are real files.
    pack_root = os.path.dirname(os.path.realpath(anchor))

    if required_suite:
        suite_path = os.path.join(pack_root, required_suite)
        if not os.path.exists(suite_path):
            raise RuntimeError(
                f"Vendored pre-compiled query pack at '{pack_root}' is "
                f"incomplete: suite '{suite_path}' is missing. Refusing to "
                "fall back to any other query source."
            )
    return pack_root


def _find_compiled_pack_root():
    """Locate the pre-compiled MISRA C++ query pack root from runfiles."""
    return _find_release_pack_root(
        COMPILED_PACK_RUNFILE,
        "@codeql_coding_standards_compiled",
        required_suite=MISRA_DEFAULT_SUITE_NAME,
    )


def _find_cert_cpp_pack_root():
    """Locate the pre-compiled CERT C++ query pack root from runfiles."""
    return _find_release_pack_root(
        CERT_CPP_COMPILED_PACK_RUNFILE,
        "@codeql_coding_standards_cert_cpp_compiled",
        required_suite=CERT_CPP_L1_SUITE_NAME,
    )


def _find_cert_c_pack_root():
    """Locate the pre-compiled CERT C query pack root from runfiles."""
    return _find_release_pack_root(
        CERT_C_COMPILED_PACK_RUNFILE,
        "@codeql_coding_standards_cert_c_compiled",
        required_suite=CERT_C_L1_SUITE_NAME,
    )


def _read_pack_identity(pack_root):
    """Return the (name, version) declared in the pack's qlpack.yml.

    The codeql-coding-standards user manual recommends referencing a downloaded
    release pack by its `<name>@<version>:<suite>` specifier (with the pack made
    discoverable via --search-path) rather than by a bare suite path. Reading the
    declared identity here lets CodeQL validate the pack's name and version, so
    an accidental pack/version drift fails loudly instead of silently analyzing
    whatever suite happens to live at a path.
    """
    name = None
    version = None
    with open(os.path.join(pack_root, "qlpack.yml")) as handle:
        for line in handle:
            stripped = line.strip()
            if name is None and stripped.startswith("name:"):
                name = stripped.split(":", 1)[1].strip().strip("'\"")
            elif version is None and stripped.startswith("version:"):
                version = stripped.split(":", 1)[1].strip().strip("'\"")
    if not name or not version:
        raise RuntimeError(f"Could not read pack name/version from {pack_root}/qlpack.yml")
    return name, version


def create_database(code_ql_path, config_path, target, source_root, database_path, build_configs=None):
    """Create the CodeQL database: init, build with tracing, finalize.

    ``build_configs`` is an optional list of additional Bazel ``--config`` names
    layered on top of the base ``codeql`` config for the traced build. For
    example ``["qnx"]`` produces ``bazel build --config=codeql --config=qnx``,
    which retargets the traced compilation at the QNX platform + QCC toolchain
    (see ``//.bazelrc`` ``common:qnx``). With no extra configs the build is the
    unchanged Linux analysis.
    """
    subprocess.run(
        f"{code_ql_path} database init --overwrite --begin-tracing --language=cpp "
        f"--codescanning-config={config_path} --source-root={source_root} -- {database_path}",
        shell=True,
        check=True,
    )

    env_file = os.path.join(database_path, "temp/tracingEnvironment/start-tracing.json")
    with open(env_file) as f:
        codeql_env = json.load(f)
    env = _get_merged_environment(codeql_env)

    # Process coding standards config
    subprocess.run(
        f"bazel run @codeql_coding_standards//:process_coding_standards_config -- --working-dir={source_root}",
        shell=True,
        env=env,
        cwd=source_root,
        check=True,
    )

    # Build with CodeQL tracing
    timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
    bazel_cmd = f"bazel build --config=codeql --stamp --action_env=CODEQL_SEED_FORCE_RECOMPILE={timestamp}"
    for extra_config in build_configs or []:
        bazel_cmd += f" --config={extra_config}"
    bazel_cmd += _get_action_env_extension(codeql_env)
    subprocess.run(f"{bazel_cmd} {target}", shell=True, env=env, cwd=source_root, check=True)

    # Finalize database
    subprocess.run(
        f"{code_ql_path} database finalize -j=0 -- {database_path}",
        shell=True,
        check=True,
    )


def _build_report_analysis_spec(report, source_root):
    """Build the query targets and pack search paths for the selected checks.

    ``report`` is a list of check names (see REPORT_CHOICES). Each check maps to
    one query pack/suite specifier; all of them are passed to a single
    `codeql database analyze` invocation (it accepts several and produces one
    combined SARIF, so multiple checks run as ONE analysis rather than several
    that get merged afterwards). The AlertSuppression helper query is always
    appended so in-source `// codeql[...]` suppressions work for whichever
    checks are selected.

    Returns a dict with:
      - query_targets: list of query/suite/pack specifiers passed to
        `codeql database analyze`.
      - search_paths: list of pre-compiled release-pack roots for --search-path.
      - additional_packs: list of directories (containing uncompiled qlpack.yml
        dirs) for --additional-packs.
      - run_coding_standards_reports: whether to run the codeql-coding-standards
        analysis_report/recategorize tooling afterwards (True when at least one
        MISRA/CERT suite is selected, i.e. coding-standards guidelines).

    See REPORT_CHOICES / the --report flag.
    """
    query_targets = []
    search_paths = []
    additional_packs = []
    run_coding_standards_reports = False

    for check in report:
        if check == "misra-default":
            pack_root = _find_compiled_pack_root()
            pack_name, pack_version = _read_pack_identity(pack_root)
            query_targets.append(f"{pack_name}@{pack_version}:{MISRA_DEFAULT_SUITE_NAME}")
            search_paths.append(pack_root)
            run_coding_standards_reports = True
        elif check == "cert-cpp-l1":
            pack_root = _find_cert_cpp_pack_root()
            pack_name, pack_version = _read_pack_identity(pack_root)
            query_targets.append(f"{pack_name}@{pack_version}:{CERT_CPP_L1_SUITE_NAME}")
            search_paths.append(pack_root)
            run_coding_standards_reports = True
        elif check == "cert-c-l1":
            pack_root = _find_cert_c_pack_root()
            pack_name, pack_version = _read_pack_identity(pack_root)
            query_targets.append(f"{pack_name}@{pack_version}:{CERT_C_L1_SUITE_NAME}")
            search_paths.append(pack_root)
            run_coding_standards_reports = True
        elif check == "cpp-code-scanning":
            query_targets.append(CPP_CODE_SCANNING_SPEC)
        elif check == "code-complexity":
            complexity_pack_dir = os.path.join(source_root, COMPLEXITY_PACK_RELATIVE_DIR)
            if not os.path.isfile(os.path.join(complexity_pack_dir, "qlpack.yml")):
                raise RuntimeError(
                    f"Local code-complexity query pack not found at '{complexity_pack_dir}'."
                )
            query_targets.append(COMPLEXITY_SUITE_SPEC)
            # --additional-packs takes the PARENT directory of the pack (it
            # searches subdirectories for qlpack.yml files matching the
            # specifier's pack name), i.e. third_party/codeql, not the
            # code_complexity dir itself.
            additional_packs.append(os.path.dirname(complexity_pack_dir))
        else:
            raise ValueError(f"Unknown report preset: {check!r}")

    # AlertSuppression is a helper, not a selectable check: it enables
    # `// codeql[...]` in-source suppressions for the checks above.
    query_targets.append(ALERT_SUPPRESSION_SPEC)

    # De-duplicate while preserving order (multiple checks can share a pack).
    search_paths = list(dict.fromkeys(search_paths))
    additional_packs = list(dict.fromkeys(additional_packs))

    return {
        "query_targets": query_targets,
        "search_paths": search_paths,
        "additional_packs": additional_packs,
        "run_coding_standards_reports": run_coding_standards_reports,
    }


def analyze_database(
    code_ql_path,
    database_path,
    source_root,
    analysis_report_path=None,
    recategorize_path=None,
    coding_standards_config_path=None,
    query_spec=None,
    report=DEFAULT_REPORTS,
    rerun=False,
    output_prefix="codeql",
    output_dir=None,
):
    """Run CodeQL analysis and generate MISRA/CERT C++ compliance reports.

    ``rerun`` passes ``--rerun`` to ``codeql database analyze``, forcing every
    query to be evaluated even when a cached BQRS result already exists in the
    database (``<database>/results/<pack>/<pack>/<query>.bqrs``). CodeQL reuses
    those cached results whenever it considers a query unchanged, and it does
    NOT detect an edit to the source of a locally analyzed (uncompiled) query
    pack, so iterating on such queries without ``--rerun`` silently reports
    stale findings. Irrelevant for CI, where every run uses a freshly created
    database with an empty results cache.
    """
    output_base = output_dir or _get_bazel_info(source_root).get("output_path")
    os.makedirs(output_base, exist_ok=True)

    # Analyze against the pre-compiled query pack(s) for the selected report
    # preset (see _build_report_analysis_spec / REPORT_CHOICES). Following the
    # codeql-coding-standards user manual's recommended approach for released
    # pack artifacts, each pack is referenced by its `<name>@<version>:<suite>`
    # specifier and made discoverable via --search-path (or, for the local
    # uncompiled complexity pack, via --additional-packs). The pre-compiled
    # packs already contain the compiled queries and all their library
    # dependencies, so this runs exactly the pinned ruleset without
    # recompiling the queries and without downloading anything from the
    # registry.
    #
    # --query-spec overrides all of this to analyze a single query straight
    # from the vendored coding-standards sources (used for debugging
    # individual rules), taking priority over --report.
    if query_spec:
        query_targets = [query_spec]
        analyze_flags = [f"--additional-packs={_find_coding_standards_root()}"]
        run_coding_standards_reports = True
    else:
        spec = _build_report_analysis_spec(report, source_root)
        query_targets = spec["query_targets"]
        analyze_flags = []
        if spec["search_paths"]:
            analyze_flags.append(f"--search-path={':'.join(spec['search_paths'])}")
        if spec["additional_packs"]:
            analyze_flags.append(f"--additional-packs={':'.join(spec['additional_packs'])}")
        run_coding_standards_reports = spec["run_coding_standards_reports"]
    query_arg = " " + " ".join(query_targets)
    common_analyze_flags = " ".join(analyze_flags)
    # --rerun forces re-evaluation of queries whose BQRS results are already
    # cached in the database; needed when iterating on query sources (see the
    # `rerun` parameter docstring above). Off by default: CI always analyzes a
    # freshly created database, so the cache is empty there anyway.
    rerun_flag = " --rerun" if rerun else ""
    sarif_path = f"{output_base}/{output_prefix}.sarif"

    # Run CodeQL analysis, producing SARIF only. The merged/deduplicated
    # union SARIF (see @sarif_multitool//:sarif_multitool_cli in
    # _codeql.yml) is published and consumed directly by the quality
    # dashboard afterward; no CSV is ever generated.
    print("\n Running CodeQL analysis...")
    subprocess.run(
        f"{code_ql_path} database analyze -j=0{rerun_flag} {database_path}{query_arg} "
        f"{common_analyze_flags} "
        f"--format=sarifv2.1.0 --output={sarif_path}",
        shell=True,
        check=True,
    )

    if run_coding_standards_reports:
        recategorize_sarif(
            recategorize_path,
            coding_standards_config_path,
            sarif_path,
        )
    normalize_sarif_rule_order(sarif_path)
    _add_suppression_justifications(sarif_path, source_root)

    # Generate reports using CodeQL analysis_report tool. Only meaningful for
    # reports that include at least one codeql-coding-standards suite (MISRA
    # and/or CERT); skipped for the generic code-scanning-only run and for the
    # local complexity report, whose rule IDs aren't coding-standards guidelines.
    if run_coding_standards_reports and analysis_report_path and os.path.exists(analysis_report_path):
        print(" Generating MISRA/CERT compliance reports...")
        try:
            # Make analysis_report executable and run it
            os.chmod(analysis_report_path, 0o755)

            # Remove existing reports directory if it exists
            reports_output_dir = os.path.join(output_base, "analysis_reports")
            if os.path.exists(reports_output_dir):
                shutil.rmtree(reports_output_dir)

            # Prepare environment with CodeQL binary path so analysis_report can find 'codeql' command
            env = os.environ.copy()
            codeql_bin_dir = os.path.dirname(os.path.realpath(code_ql_path))
            print(f" Resolved CodeQL bin dir: {codeql_bin_dir}")
            print(f" CodeQL bin dir exists: {os.path.isdir(codeql_bin_dir)}")
            env["PATH"] = f"{codeql_bin_dir}:{env.get('PATH', '')}"
            print(f" PATH for analysis_report: {env['PATH']}")

            # analysis_report expects positional args: database-dir sarif-file output-dir

            result = subprocess.run(
                [analysis_report_path, database_path, sarif_path, reports_output_dir],
                capture_output=True,
                text=True,
                env=env,
            )

            # Always show subprocess output for diagnostics
            if result.stdout:
                print(f" [analysis_report stdout]: {result.stdout.strip()}")

            if result.stderr:
                print(f" [analysis_report stderr]: {result.stderr.strip()}")
            if result.returncode != 0:
                print(f"   analysis_report exited with code {result.returncode}")
                # Don't raise exception - allow workflow to continue

        except Exception as e:
            print(f"Report generation exception: {e}")


def recategorize_sarif(recategorize_path, coding_standards_config_path, sarif_path):
    if not recategorize_path:
        return sarif_path
    if not coding_standards_config_path or not os.path.isfile(coding_standards_config_path):
        raise RuntimeError(f"Coding standards config file not found: {coding_standards_config_path!r}")

    recategorize_path = os.path.realpath(recategorize_path)
    coding_standards_schema_path, sarif_schema_path = _find_recategorization_schema_paths()
    coding_standards_schema_path = os.path.realpath(coding_standards_schema_path)
    sarif_schema_path = os.path.realpath(sarif_schema_path)
    recategorized_sarif_path = f"{sarif_path}.recategorized"
    subprocess.run(
        [
            recategorize_path,
            "--coding-standards-schema-file",
            coding_standards_schema_path,
            "--sarif-schema-file",
            sarif_schema_path,
            coding_standards_config_path,
            sarif_path,
            recategorized_sarif_path,
        ],
        check=True,
    )
    os.replace(recategorized_sarif_path, sarif_path)
    return sarif_path


_CODEQL_SUPPRESSION_RE = re.compile(
    r"codeql\s*\[\s*(?P<rule_id>[^\]\s]+)\s*\]\s*(?P<justification>.*)",
    re.IGNORECASE,
)


def _resolve_source_path(uri, source_root):
    """Resolve a SARIF artifact URI to an absolute filesystem path."""
    if uri.startswith("file://"):
        return urllib.parse.unquote(urllib.parse.urlparse(uri).path)
    if uri.startswith("file:"):
        return urllib.parse.unquote(uri[len("file:"):])
    if os.path.isabs(uri):
        return uri
    return os.path.join(source_root, uri)


def _find_suppression_justification(result, rule_id, source_root):
    """Return the trailing text of the ``// codeql[<id>]`` comment suppressing ``result``.

    ``AlertSuppression.ql`` only records the ``codeql[...]`` annotation (the rule
    id) and drops whatever follows the closing ``]``. The comment sits on the
    line immediately above the finding, so this reads that line back from the
    source and returns the justification text (or ``None`` when there is none).
    """
    location = (result.get("locations") or [{}])[0]
    physical = location.get("physicalLocation") or {}
    artifact = physical.get("artifactLocation") or {}
    uri = artifact.get("uri")
    region = physical.get("region") or {}
    start_line = region.get("startLine")
    if not uri or not start_line:
        return None

    path = _resolve_source_path(uri, source_root)
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as source_file:
            lines = source_file.readlines()
    except OSError:
        return None

    # A `codeql[...]` suppression comment applies to the line directly below it,
    # so it lives on the line immediately above the finding's start line.
    comment_index = start_line - 2  # 0-based index of line (startLine - 1)
    if comment_index < 0 or comment_index >= len(lines):
        return None

    match = _CODEQL_SUPPRESSION_RE.search(lines[comment_index].strip())
    if not match or match.group("rule_id") != rule_id:
        return None
    justification = match.group("justification").strip()
    return justification or None


def _add_suppression_justifications(sarif_path, source_root):
    """Annotate in-source suppressions with the justification from the comment.

    Populates ``suppressions[].justification`` on results suppressed via a
    ``// codeql[<query-id>] <justification>`` comment, so the rationale travels
    with the finding in the SARIF. Rewrites ``sarif_path`` in place only when at
    least one justification was added.
    """
    with open(sarif_path, "r", encoding="utf-8") as sarif_file:
        sarif = json.load(sarif_file)

    changed = False
    for run in sarif.get("runs", []):
        rule_id_by_index = {
            index: rule.get("id")
            for index, rule in enumerate(run.get("tool", {}).get("driver", {}).get("rules", []))
        }
        for result in run.get("results", []):
            suppressions = result.get("suppressions")
            if not suppressions:
                continue
            rule_id = result.get("ruleId")
            if not rule_id:
                rule_id = rule_id_by_index.get(result.get("ruleIndex"))
            if not rule_id:
                continue
            justification = _find_suppression_justification(result, rule_id, source_root)
            if not justification:
                continue
            for suppression in suppressions:
                if suppression.get("kind") == "inSource" and "justification" not in suppression:
                    suppression["justification"] = justification
                    changed = True

    if changed:
        justified_path = f"{sarif_path}.justified"
        with open(justified_path, "w", encoding="utf-8") as sarif_file:
            json.dump(sarif, sarif_file, indent=2)
            sarif_file.write("\n")
        os.replace(justified_path, sarif_path)
    return sarif_path


def normalize_sarif_rule_order(sarif_path):
    with open(sarif_path, "r", encoding="utf-8") as sarif_file:
        sarif = json.load(sarif_file)

    runs = sarif.get("runs", [])
    if not runs:
        return sarif_path

    for run in runs:
        _normalize_sarif_run(run)

    normalized_sarif_path = f"{sarif_path}.normalized"
    with open(normalized_sarif_path, "w", encoding="utf-8") as sarif_file:
        json.dump(sarif, sarif_file, indent=2)
        sarif_file.write("\n")
    os.replace(normalized_sarif_path, sarif_path)
    return sarif_path


def _normalize_sarif_run(run):
    driver = run.get("tool", {}).get("driver", {})
    rules = driver.get("rules", [])
    ordered_rules = sorted(
        enumerate(rules),
        key=lambda indexed_rule: indexed_rule[1].get("id", ""),
    )
    index_map = {old_index: new_index for new_index, (old_index, _) in enumerate(ordered_rules)}
    driver["rules"] = [rule for _, rule in ordered_rules]

    artifacts = run.get("artifacts", [])
    ordered_artifacts = sorted(
        enumerate(artifacts),
        key=lambda indexed_artifact: indexed_artifact[1].get("location", {}).get("uri", ""),
    )
    artifact_index_map = {old_index: new_index for new_index, (old_index, _) in enumerate(ordered_artifacts)}
    run["artifacts"] = [artifact for _, artifact in ordered_artifacts]

    for result in run.get("results", []):
        if "ruleIndex" in result:
            result["ruleIndex"] = index_map[result["ruleIndex"]]
        result_rule = result.get("rule")
        if result_rule is not None and "index" in result_rule:
            result_rule["index"] = index_map[result_rule["index"]]

    for key, value in run.items():
        if key != "artifacts":
            _remap_artifact_indices(value, artifact_index_map)


def _remap_artifact_indices(value, artifact_index_map):
    if isinstance(value, dict):
        artifact_location = value.get("artifactLocation")
        if isinstance(artifact_location, dict) and "index" in artifact_location:
            if artifact_location.get("uri"):
                del artifact_location["index"]
            else:
                artifact_location["index"] = artifact_index_map[artifact_location["index"]]
        for child in value.values():
            _remap_artifact_indices(child, artifact_index_map)
    elif isinstance(value, list):
        for child in value:
            _remap_artifact_indices(child, artifact_index_map)


def _find_recategorization_schema_paths():
    from python.runfiles import Runfiles

    runfiles = Runfiles.Create()
    coding_standards_schema_path = runfiles.Rlocation(
        "codeql_coding_standards/schemas/coding-standards-schema-1.0.0.json"
    )
    sarif_schema_path = runfiles.Rlocation("codeql_coding_standards/schemas/sarif-schema-2.1.0.json")
    if not coding_standards_schema_path or not os.path.isfile(coding_standards_schema_path):
        raise RuntimeError("Failed to load Coding Standards schema!")
    if not sarif_schema_path or not os.path.isfile(sarif_schema_path):
        raise RuntimeError("Failed to load Sarif schema!")
    return coding_standards_schema_path, sarif_schema_path


def main():
    parser = argparse.ArgumentParser(description="Run CodeQL linting operations")
    parser.add_argument("--codeql_path", help="Path to CodeQL binary")
    parser.add_argument("--config_path", help="CodeQL config file")
    parser.add_argument("--analysis_report_path", help="Path to analysis_report binary")
    parser.add_argument(
        "--guideline_recategorize_path",
        help="Path to guideline recategorization binary",
    )
    parser.add_argument("--target", nargs="+", help="Bazel targets to build")
    parser.add_argument(
        "--phase",
        choices=["create-database", "analyze-database", "all"],
        default="all",
        help="Execution phase",
    )
    parser.add_argument("--database-path", help="CodeQL database path")
    parser.add_argument("--query-spec", help="CodeQL query spec")
    parser.add_argument(
        "--report",
        nargs="+",
        choices=REPORT_CHOICES,
        default=DEFAULT_REPORTS,
        help="One or more checks to run (space-separated). When omitted, only "
        "'misra-default' (the MISRA C++ default suite) is run. Available "
        "checks: misra-default, cert-cpp-l1, cert-c-l1, cpp-code-scanning, "
        "code-complexity. The AlertSuppression helper query is always added so "
        "'// codeql[...]' suppressions work. Ignored when --query-spec is "
        "given.",
    )
    parser.add_argument(
        "--rerun",
        action="store_true",
        help="Pass --rerun to 'codeql database analyze' so queries are "
        "re-evaluated even when a BQRS result is already cached in the "
        "database. Needed when iterating on query sources (e.g. the local "
        "code-complexity pack), because CodeQL otherwise reuses stale cached "
        "results after a query file changes. No effect in CI, where the "
        "database is always freshly created.",
    )
    parser.add_argument("--output-prefix", default="codeql", help="Output prefix")
    parser.add_argument("--output-dir", help="Output directory (default: the repo root)")
    parser.add_argument(
        "--build-config",
        action="append",
        default=[],
        dest="build_configs",
        metavar="CONFIG",
        help="Additional Bazel --config to layer on the traced build (repeatable). "
        "E.g. --build-config qnx runs 'bazel build --config=codeql --config=qnx'.",
    )

    args = parser.parse_args()
    target = " ".join(args.target) if args.target else ""
    source_root = os.environ["BUILD_WORKING_DIRECTORY"]

    # Resolve a relative --output-dir against the repo root. `bazel run` starts
    # the tool with the target's runfiles directory as its working directory
    # (not the workspace), so a bare relative path like `./_SCA/...` would
    # otherwise land deep inside the Bazel output tree instead of the repo.
    if args.output_dir and not os.path.isabs(args.output_dir):
        args.output_dir = os.path.join(source_root, args.output_dir)

    # When --output-dir is omitted, write results into the repo root rather than
    # the Bazel output tree (the old default was `bazel info output_path`, which
    # lives under the Bazel cache and is easy to lose for local runs).
    if not args.output_dir:
        args.output_dir = source_root

    # Make codeql_path absolute
    codeql_path = os.path.abspath(args.codeql_path) if args.codeql_path else None
    coding_standards_config_path = os.path.join(source_root, CODING_STANDARDS_CONFIG_RELATIVE_PATH)
    if not os.path.isabs(coding_standards_config_path):
        coding_standards_config_path = os.path.abspath(coding_standards_config_path)

    if args.phase == "create-database":
        os.makedirs(os.path.dirname(args.database_path), exist_ok=True)
        create_database(
            codeql_path,
            args.config_path,
            target,
            source_root,
            args.database_path,
            build_configs=args.build_configs,
        )

    elif args.phase == "analyze-database":
        analyze_database(
            codeql_path,
            args.database_path,
            source_root,
            analysis_report_path=args.analysis_report_path,
            recategorize_path=args.guideline_recategorize_path,
            coding_standards_config_path=coding_standards_config_path,
            query_spec=args.query_spec,
            report=args.report,
            rerun=args.rerun,
            output_prefix=args.output_prefix,
            output_dir=args.output_dir,
        )

    else:  # all
        # Use standard Bazel output directory for database
        bazel_info = _get_bazel_info(source_root)
        output_path = args.output_dir or bazel_info.get("output_path")
        # Ensure the output directory exists before CodeQL tries to create the
        # database inside it (codeql database init does not create parents).
        os.makedirs(output_path, exist_ok=True)
        os.makedirs(TMP_PATH_FOR_DATABASES, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TMP_PATH_FOR_DATABASES) as database_location:
            create_database(
                codeql_path,
                args.config_path,
                target,
                source_root,
                database_location,
                build_configs=args.build_configs,
            )
            analyze_database(
                codeql_path,
                database_location,
                source_root,
                analysis_report_path=args.analysis_report_path,
                recategorize_path=args.guideline_recategorize_path,
                coding_standards_config_path=coding_standards_config_path,
                query_spec=args.query_spec,
                report=args.report,
                rerun=args.rerun,
                output_prefix=args.output_prefix,
                output_dir=args.output_dir,
            )


def _get_action_env_extension(codeql_env):
    action_env_extension = ""
    for env_var in codeql_env:
        action_env_extension += f" --action_env={env_var}"
    return action_env_extension


def _get_merged_environment(codeql_env):
    env = os.environ.copy()
    for var in codeql_env:
        env[var] = f"{codeql_env[var]}:{env.get(var, '')}" if var in env else codeql_env[var]
    return env


def _get_bazel_info(source_root):
    result = subprocess.run(
        "bazel info",
        shell=True,
        cwd=source_root,
        capture_output=True,
        text=True,
        check=True,
    )

    # Parse the output into a dictionary
    bazel_info = {}
    for line in result.stdout.strip().split("\n"):
        if ":" in line:
            key, value = line.split(":", 1)
            bazel_info[key.strip()] = value.strip()
    return bazel_info


if __name__ == "__main__":
    main()

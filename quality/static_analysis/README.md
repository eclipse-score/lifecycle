# Static Code Analysis (CodeQL / MISRA / CERT)

This directory hosts the repo's hermetic CodeQL static-analysis pipeline
(`//quality/static_analysis:codeql_lint`, implemented in `codeql_lint.py`).

`bazel run //quality/static_analysis:codeql_lint` builds a CodeQL database from a **traced**
`bazel build`, runs CodeQL queries over it, and writes a **SARIF** file plus (for MISRA/CERT)
compliance reports. Everything — CodeQL CLI, coding-standards packs, SARIF tooling — is pinned by
URL+sha256 in `MODULE.bazel`; nothing is compiled or downloaded at analysis time.

## Quick start

```bash
# Default: MISRA C++ default suite only (misra-cpp-default.qls)
bazel run //quality/static_analysis:codeql_lint -- \
  --target //score/health_monitor/src/cpp:common
```

Run several specific checks in one go — CERT C++ L1 + CERT C L1 + cpp-code-scanning + complexity:

```bash
bazel run --config=x86_64-linux //quality/static_analysis:codeql_lint -- \
  --report cert-cpp-l1 cert-c-l1 cpp-code-scanning code-complexity \
  --target $(bazel query 'kind("cc_library|cc_binary", //score/...)')
```

> The check is spelled **`code-complexity`** (hyphen) on the command line. The *directory* of the
> local pack is `third_party/codeql/code_complexity` (underscore) — that's just a filesystem name;
> the `--report` value uses the hyphenated pack name.

## CLI reference

| Flag | Meaning |
| --- | --- |
| `--target TARGET...` | Bazel targets to analyze (required) |
| `--report CHECK...` | Checks to run (space-separated; default `misra-default`). See table below. |
| `--phase {create-database,analyze-database,all}` | Build DB, analyze, or both (default `all`) |
| `--database-path PATH` | DB location (must be under `/var/tmp`; `create-/analyze-database` phases) |
| `--output-dir DIR` | Where the SARIF + `analysis_reports/` are written (default: the repo root; relative paths resolve against it) |
| `--output-prefix P` | SARIF filename prefix (default `codeql`) |
| `--rerun` | Re-evaluate queries even if cached BQRS results exist (needed after editing query sources) |
| `--query-spec SPEC` | Run a single query from the coding-standards sources (overrides `--report`) |
| `--build-config CONFIG` | Extra Bazel `--config` for the traced build (repeatable; e.g. `qnx`) |

### The `--report` checks

| check | what it runs | rules |
| --- | --- | --- |
| `misra-default` | MISRA C++ default suite (`misra-cpp-default.qls`) | 218 |
| `cpp-code-scanning` | CodeQL security/quality suite (`cpp-code-scanning.qls`) | 57 |
| `cert-cpp-l1` | CERT C++ L1 (`cert-cpp-l1.qls`) | 23 |
| `cert-c-l1` | CERT C L1 (`cert-c-l1.qls`) | 18 |
| `code-complexity` | local metrics pack (`code-complexity-queries:suites/thresholds.qls`) | 6 |

All selected checks run in **one** `codeql database analyze` invocation → one SARIF.
`AlertSuppression.ql` is **always added automatically** (it is not a selectable check); it makes the
inline `// codeql[...]` suppressions below work.

## Outputs

- `<output-dir>/<output-prefix>.sarif` — the findings.
- `<output-dir>/analysis_reports/` — MISRA/CERT compliance reports (only when a MISRA/CERT check is
  selected): `guideline_compliance_summary.md`, `deviations_report.md`,
  `guideline_recategorizations_report.md`, `database_integrity_report.md`.

## Deviations and suppressions — two mechanisms

There are two independent ways to waive a finding. They differ in **where** the waiver lives and
**how** it surfaces.

### A. In-source suppressions — `AlertSuppression.ql` (`// codeql[...]`)

A comment placed directly in the source, next to the offending construct:

```cpp
// codeql[cpp/misra/no-implicit-bool-conversion] Legacy code, intentional implicit conversion
bool enabled = some_integer;
```

- The `[...]` must contain the exact SARIF `ruleId` (e.g. `cpp/misra/no-implicit-bool-conversion`).
- Put the comment on the line **above** the finding (or trailing on the same line).
- `AlertSuppression.ql` (run automatically) marks the finding `suppressed`; the text **after** the
  `]` is copied by the tool into the SARIF as the suppression `justification`:

  ```json
  "suppressions": [ { "kind": "inSource", "justification": "Legacy code, intentional implicit conversion" } ]
  ```

Use this for **ad-hoc, code-local** waivers that travel with the code.

### B. Central deviations — `coding-standards.yaml`

A deviation is declared centrally in `quality/static_analysis/coding-standards.yaml` and waives a
finding (identified by `rule-id` + `query-id`) with a written `justification` — no source edit
needed, or a tiny marker when scoping to a specific location. Examples are shown (commented out) in
that file; currently `deviations: []`.

```yaml
# Whole-directory deviation:
deviations:
  - rule-id: "RULE-18-5-2"
    query-id: "cpp/misra/avoid-program-terminating-functions"
    justification: "Fail-silent design: termination is the agreed safe-state."
    paths:
      - "score/**"

# Location-scoped deviation via a code identifier:
  - rule-id: "RULE-6-8-3"
    query-id: "cpp/misra/automatic-storage-assigned-to-object-greater-lifetime"
    code-identifier: "unix-domain-iovec-stack-buffers"
    scope: "Applies only to the marked iovec assignments."
    justification: "Scatter/gather I/O read synchronously before return."
```

The `code-identifier` is then referenced in the source with a marker (standard = `misra`, `cert` or
`autosar`, matching the query):

```cpp
// codeql::misra_deviation(unix-domain-iovec-stack-buffers)           # this statement
// codeql::misra_deviation_next_line(unix-domain-iovec-stack-buffers) # next line
// codeql::misra_deviation_begin(unix-domain-iovec-stack-buffers)     # range start
// codeql::misra_deviation_end(unix-domain-iovec-stack-buffers)       # range end

[[codeql::misra_deviation("unix-domain-iovec-stack-buffers")]]        # attribute form
```

Deviations surface in **`analysis_reports/deviations_report.md`** (not in the SARIF `suppressions`).
Optional record fields: `raised-by` / `approved-by` (`{name, date}`) and `permit-id`.

### Which to use?

| | `// codeql[...]` (AlertSuppression.ql) | `coding-standards.yaml` deviations |
| --- | --- | --- |
| Where declared | inline in the source | centrally in the config |
| Scope | the construct under the comment | whole subtree (`paths`) or a marked location (`code-identifier`) |
| Where it shows up | SARIF `suppressions[].justification` | `deviations_report.md` |
| Best for | code-local, one-off waivers | process-level, reviewed/approved waivers |

## Gotchas

- **Database must be under `/var/tmp`** (`--sandbox_writable_path=/var/tmp`); anywhere else gives a
  silently-empty DB with 0 findings.
- **Don't use bare `--target //score/...` in the 22.04 sandbox** — the Rust targets need a newer
  glibc. Use `$(bazel query 'kind("cc_library|cc_binary", //score/...)')` (129 production targets).
- **`--rerun`** after editing local query sources (`code-complexity`): `database analyze` reuses
  cached BQRS results and won't pick up query edits otherwise.
- **`--query-spec`** to debug a single rule, e.g. `--query-spec cpp/misra/no-implicit-bool-conversion`.

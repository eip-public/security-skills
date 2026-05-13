---
name: code-codeql-analysis
description: Build, validate, and analyze CodeQL databases for source trees or decompiler-exported source. Use when running CodeQL scans, testing CodeQL readiness, normalizing Ghidra/decompiler C/C++ output for CodeQL, or performing deep interprocedural security analysis after manual review.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, codeql, static-analysis, decompiler, ghidra, variant-hunting]
    related_skills: [code-variant-analysis, code-semgrep-hunting, design-sharp-edges]
platforms: [macos, linux]
role: helper
branch_axis: n/a
mcp_required: []
mcp_optional: [ghidra]
tools_required: [codeql]
tools_optional: [git, jq, python3, gcc, clang, ghidraRun]
helper_skills: [code-variant-analysis, code-semgrep-hunting, design-sharp-edges]
tags: [security, codeql, static-analysis, decompiler, ghidra, variant-hunting]
---

# CodeQL Analysis for Source and Decompiled Code

## Routing doctrine reference
Follow `../ROUTING-DOCTRINE.md` as the house style for:
- branch vs helper ownership
- explicit load triggers
- explicit `When NOT to Use` boundaries
- normalized `=== SKILL HANDOFF ===` blocks
- sequential phase loading instead of preloading downstream skills

## Scope
This skill adapts general CodeQL workflow ideas from the public Trail of Bits CodeQL skill (`https://agentskills.me/skill/codeql` / `trailofbits/skills`) to this repository's CVE-routing style, and adds a reusable workflow for **decompiler-exported C/C++** from Ghidra or similar tools.

It is intentionally not tied to one CVE or one binary. Use it for any source tree or any decompiled-source corpus where CodeQL may be useful.

Supporting files:
- `templates/decompiler_compat.h` is a starter compatibility shim to copy into `$OUTPUT_DIR/shims/` and extend per target.

## When to Use
- User asks to run, test, validate, or troubleshoot CodeQL.
- You need deep interprocedural/static analysis beyond grep or Semgrep.
- You have source code and want a repeatable CodeQL security scan.
- You have Ghidra/decompiler output and want to determine whether it can be normalized enough for CodeQL.
- `code-variant-analysis` needs call graph/data-flow coverage after a root cause is understood.
- A CVE branch skill needs evidence from static analysis before PoC validation or reporting.

## When NOT to Use
- Quick literal or regex searches: use ripgrep first.
- Simple syntactic pattern matching or unbuildable snippets where Semgrep is sufficient.
- Custom QL query development as the main task, unless the query is small and directly supports the current analysis.
- Claims of exploitability: CodeQL results still need manual reachability and PoC validation.
- Raw decompiled C/C++ with severe syntax errors and no normalization budget; first run the decompiler-normalization gate below.

## Primary Output
A CodeQL analysis directory containing:
- database marker: `codeql-database.yml`
- build/extraction logs
- diagnostics and quality notes
- raw SARIF results
- triaged findings markdown
- normalization shims if analyzing decompiled C/C++

Default artifact path inside CVE labs:

```text
~/exploit-intel/labs/CVE-YYYY-NNNNN/artifacts/codeql/
```

For non-CVE work, use a local auto-incremented directory:

```text
./static_analysis_codeql_1/
./static_analysis_codeql_2/
...
```

## Minimum Viable Path
1. Verify CodeQL and query packs.
2. Resolve one output directory and keep all artifacts inside it.
3. Discover existing CodeQL databases before building new ones.
4. Build or reuse a database.
5. Assess database quality before trusting results.
6. Run an explicit query suite and save SARIF.
7. Parse SARIF, triage findings, and document limitations.
8. For decompiled C/C++, run syntax/normalization gates and clearly label confidence.

## Scope control for smoke tests
If the user asks for a **simple CodeQL test**, keep the first pass deliberately small: verify install, build one minimal database, run one explicit suite, summarize pass/fail, and stop unless the result exposes a blocker. Do not silently expand into a full multi-turn audit. If the test uncovers setup gaps or decompiler normalization blockers, say so concisely and ask/confirm before broadening scope.

## Setup Audit
Always start with an audit. Do not assume CodeQL is installed or that packs are available.

```bash
command -v codeql || echo "MISSING: codeql"
codeql version
codeql resolve languages
codeql resolve qlpacks --format=json > "$OUTPUT_DIR/qlpacks.json"
```

For C/C++ security scans, ensure the official pack exists:

```bash
codeql resolve qlpacks | grep -F 'codeql/cpp-queries' || \
  codeql pack download codeql/cpp-queries
```

If components are missing on this environment, the canonical host
installer is [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes)
(see `installers/hermes/components/75-runtime-langs.sh` and `80-re.sh`
for CodeQL and Ghidra respectively). Patch the relevant component
there rather than installing tools inline from this skill.

## Output Directory Discipline
Resolve `$OUTPUT_DIR` once, then put everything under it.

```bash
BASE="static_analysis_codeql"
N=1
while [ -e "${BASE}_${N}" ]; do N=$((N + 1)); done
OUTPUT_DIR="${BASE}_${N}"
mkdir -p "$OUTPUT_DIR" "$OUTPUT_DIR/raw" "$OUTPUT_DIR/results" "$OUTPUT_DIR/logs" "$OUTPUT_DIR/diagnostics"
```

Recommended layout:

```text
$OUTPUT_DIR/
├── codeql.db/                  # database directory
├── build.log                   # build/extraction log
├── qlpacks.json                # pack inventory
├── raw/results.sarif           # unfiltered CodeQL output
├── results/findings.md         # triaged report
├── diagnostics/                # quality checks and generated summaries
├── normalized/                 # normalized decompiled files, if applicable
└── shims/                      # compatibility headers, if applicable
```

## Existing Database Discovery
A CodeQL database is identified by a `codeql-database.yml` marker. Never assume the directory is named `codeql.db`.

```bash
find . -maxdepth 3 -name codeql-database.yml -not -path '*/.*' 2>/dev/null \
  | while read -r yml; do dirname "$yml"; done
```

If multiple databases exist and the user did not name one, ask which to reuse or whether to build a new one. If the user explicitly asked to build a new database, skip the prompt and build.

## Source Tree Workflow
Use this for normal source code with a build system or interpretable language.

### 1. Select language

```bash
codeql resolve languages
```

Common language IDs: `cpp`, `csharp`, `go`, `java-kotlin`, `javascript-typescript`, `python`, `ruby`, `swift`.

### 2. Build database

Interpreted/no-build languages:

```bash
codeql database create "$OUTPUT_DIR/codeql.db" \
  --language=python \
  --source-root="$SRC_ROOT" \
  --overwrite
```

Compiled C/C++ with a real build:

```bash
codeql database create "$OUTPUT_DIR/codeql.db" \
  --language=cpp \
  --source-root="$SRC_ROOT" \
  --command='make clean && make -j$(nproc)' \
  --overwrite 2>&1 | tee "$OUTPUT_DIR/build.log"
```

C/C++ with no viable build, last resort only:

```bash
codeql database create "$OUTPUT_DIR/codeql.db" \
  --language=cpp \
  --source-root="$SRC_ROOT" \
  --build-mode=none \
  --overwrite 2>&1 | tee "$OUTPUT_DIR/build.log"
```

`build-mode=none` is acceptable for a smoke test or decompiled snippets, but label results as lower confidence. It does not equal a high-quality database.

### 3. Finalize if needed

```bash
codeql database finalize "$OUTPUT_DIR/codeql.db" 2>&1 | tee -a "$OUTPUT_DIR/build.log"
```

### 4. Quality gate

```bash
codeql database info "$OUTPUT_DIR/codeql.db" > "$OUTPUT_DIR/diagnostics/database-info.txt" 2>&1 || true
find "$OUTPUT_DIR/codeql.db" -name '*.trap.gz' | wc -l > "$OUTPUT_DIR/diagnostics/trap-count.txt"
find "$SRC_ROOT" -type f | wc -l > "$OUTPUT_DIR/diagnostics/source-file-count.txt"
```

Review build logs for fatal errors, cached/no-op builds, and missing files. A database that exists but extracted almost nothing is not useful.

## Decompiled C/C++ Workflow
Use this when source comes from Ghidra, RetDec, Hex-Rays, or another decompiler.

### Expectations
Decompiler output is not normal source code. Common blockers:
- pseudo-types: `undefined`, `undefined1`, `undefined2`, `undefined4`, `undefined8`
- non-standard aliases: `uint`, `ulong`, `ulonglong`, `byte`, `word`, `dword`, `qword`
- platform placeholders: `HWND`, `HINSTANCE`, `DWORD`, `LPSTR`, `BOOL`, etc.
- synthetic temporaries and casts that parse poorly
- missing headers and calling-convention macros
- gotos and dead blocks introduced by control-flow recovery

Therefore, distinguish three levels:

1. **Exportable:** decompiler produced text.
2. **Extractable:** CodeQL can create a database.
3. **Compiler-valid:** `gcc -fsyntax-only` or `clang -fsyntax-only` succeeds with shims.

Do not report level 2 as level 3.

### 1. Export decompiled code
Store raw files separately from normalized files.

```text
$OUTPUT_DIR/raw_decomp/*.c
$OUTPUT_DIR/normalized/*.c
```

With the Ghidra MCP, export representative functions with `mcp_ghidra_decomp_function` and write each result to `raw_decomp/<binary>_<function>.c`.

### 2. Create a compatibility shim
Start minimal and grow only as errors require.

```c
/* $OUTPUT_DIR/shims/decompiler_compat.h */
#ifndef DECOMPILER_COMPAT_H
#define DECOMPILER_COMPAT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define __stdcall
#define __cdecl
#define __fastcall
#define __thiscall
#define __usercall
#define __noreturn

/* Ghidra-style unknowns */
typedef uint8_t undefined;
typedef uint8_t undefined1;
typedef uint16_t undefined2;
typedef uint32_t undefined4;
typedef uint64_t undefined8;
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

/* Common decompiler aliases */
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef long long longlong;

/* Minimal Windows placeholders; extend per target as needed */
typedef void *HANDLE;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *LPVOID;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef uint32_t DWORD;
typedef int BOOL;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;

#endif
```

### 3. Normalize files
Keep raw exports immutable. Normalize by prepending the shim include.

```bash
mkdir -p "$OUTPUT_DIR/normalized" "$OUTPUT_DIR/shims"
for f in "$OUTPUT_DIR"/raw_decomp/*.c; do
  out="$OUTPUT_DIR/normalized/$(basename "$f")"
  {
    echo '#include "../shims/decompiler_compat.h"'
    echo
    cat "$f"
  } > "$out"
done
```

If raw files contain repeated generated typedefs that conflict with the shim, patch the normalized copy only.

### 4. Syntax gate
Run both GCC/Clang if available. Save logs.

```bash
gcc -fsyntax-only -w -I"$OUTPUT_DIR/shims" "$OUTPUT_DIR"/normalized/*.c \
  > "$OUTPUT_DIR/diagnostics/gcc-syntax.log" 2>&1 || true
clang -fsyntax-only -w -I"$OUTPUT_DIR/shims" "$OUTPUT_DIR"/normalized/*.c \
  > "$OUTPUT_DIR/diagnostics/clang-syntax.log" 2>&1 || true
```

Classify the corpus:
- **green:** syntax-only passes
- **yellow:** CodeQL extracts but compiler syntax fails; findings are exploratory
- **red:** CodeQL database creation fails; use Semgrep/manual review first

### 5. Build CodeQL database from normalized files

```bash
codeql database create "$OUTPUT_DIR/codeql.db" \
  --language=cpp \
  --source-root="$OUTPUT_DIR/normalized" \
  --build-mode=none \
  --overwrite 2>&1 | tee "$OUTPUT_DIR/build.log"

codeql database finalize "$OUTPUT_DIR/codeql.db" 2>&1 | tee -a "$OUTPUT_DIR/build.log"
```

### 6. Interpret decompiler findings conservatively
Expect noisy generic findings such as goto/dead-code/control-flow artifacts. Prioritize:
- attacker-controlled buffer sizes or indices
- integer overflow before allocation/copy
- unchecked allocation or null dereference in reachable paths
- command/file/network sinks if string construction is visible
- missing auth/capability checks in decompiled logic
- repeated unsafe patterns that support variant hunting

Down-rank:
- pure style findings
- `goto` findings caused by decompiler control flow
- dead code created by failed recovery
- warnings caused solely by missing types or unresolved imports

## Query Suite Selection
Prefer explicit suite references and log what you ran.

C/C++ baseline:

```bash
SUITE='codeql/cpp-queries:codeql-suites/cpp-security-and-quality.qls'
echo "$SUITE" > "$OUTPUT_DIR/rulesets.txt"
codeql database analyze "$OUTPUT_DIR/codeql.db" "$SUITE" \
  --download \
  --format=sarif-latest \
  --output="$OUTPUT_DIR/raw/results.sarif" \
  2>&1 | tee "$OUTPUT_DIR/logs/analyze.log"
```

For broader research, consider adding security-experimental suites if present, but document the precision tradeoff.

## SARIF Processing
Use Python or `jq` to summarize. Always preserve raw SARIF.

```bash
python3 - "$OUTPUT_DIR/raw/results.sarif" <<'PY' | tee "$OUTPUT_DIR/diagnostics/sarif-summary.txt"
import json, collections, pathlib, sys
sarif = pathlib.Path(sys.argv[1])
data = json.loads(sarif.read_text())
results = data.get('runs',[{}])[0].get('results',[])
rules = {r.get('id'): r for r in data.get('runs',[{}])[0].get('tool',{}).get('driver',{}).get('rules',[])}
print('total_results:', len(results))
counts = collections.Counter(r.get('ruleId') for r in results)
for rid, n in counts.most_common(25):
    rule = rules.get(rid,{})
    tags = rule.get('properties',{}).get('tags',[])
    print(f'{n:5d} {rid} tags={tags}')
PY
```

Create `results/findings.md` with:
- rule ID and severity/precision
- file/function/location
- source/sink or root-cause hypothesis
- true/false/unknown-positive triage
- decompiler limitations that affect confidence
- recommended next action

## Quality and Trust Checklist
Before reporting findings, answer:
- Did CodeQL CLI run successfully?
- Which CodeQL version and packs were used?
- Did database creation succeed and produce `codeql-database.yml`?
- How many source/decompiled files were intended vs actually extracted?
- Were build/extraction errors fatal or only warnings?
- For decompiled C/C++, did syntax-only compile pass?
- Which suite exactly was run?
- How many SARIF results were produced?
- Which results are likely decompiler artifacts?
- What manual review or PoC validation is still required?

## Standard Handoff Block
When this skill yields, emit:

```text
=== SKILL HANDOFF ===
FROM_SKILL:        code-codeql-analysis
TO_SKILL:          <next skill>
REASON:            <why CodeQL analysis is complete enough to hand off>
PRIMARY_ARTIFACTS: <OUTPUT_DIR, database path, SARIF path, findings.md, normalization shims>
STATE_READY:       <database quality, syntax gate result, result counts, top credible findings>
OPEN_QUESTIONS:    <missing types, extraction gaps, manual triage, PoC needs>
NEXT_TRIGGER:      <what evidence the next skill must produce>
====================
```

## Common Pitfalls
1. **Treating database creation as proof of quality.** A database may exist with poor extraction. Always inspect logs and counts.
2. **Letting a smoke test sprawl.** If the user asked for a simple test, report the smallest verified result first; broaden only when a blocker needs investigation or the user opts in.
3. **Calling raw decompiler output “source.”** Label it as decompiled pseudocode unless it passes syntax gates.
4. **Editing raw decompiler exports.** Keep raw exports immutable; normalize copies.
5. **Using `build-mode=none` without caveats.** It is useful for smoke tests and snippets, not equivalent to a traced build.
6. **Over-trusting generic findings.** Goto/dead-code findings are common decompiler artifacts.
7. **Scattering artifacts.** Keep every generated file inside `$OUTPUT_DIR`.
8. **Skipping secret checks after installer patches.** Any tooling script edit must be syntax-checked and scanned for accidental credentials.
9. **Reporting zero findings as clean.** Zero can mean poor extraction, wrong language, missing query packs, or an over-filtered suite.

## Verification Checklist
- [ ] Existing CodeQL skills/references were checked before creating new workflow content.
- [ ] `codeql version`, `resolve languages`, and `resolve qlpacks` were captured.
- [ ] Required query packs were installed or installer gap was documented.
- [ ] `$OUTPUT_DIR` was resolved once and used consistently.
- [ ] Existing databases were discovered by `codeql-database.yml` marker.
- [ ] Database creation/finalization succeeded or failure logs were preserved.
- [ ] Database quality was assessed before trusting findings.
- [ ] Decompiled C/C++ was normalized separately from raw exports.
- [ ] Syntax gate result was reported as green/yellow/red.
- [ ] Analysis used explicit suite references and saved SARIF.
- [ ] SARIF was summarized and triaged, not dumped unreviewed.
- [ ] Handoff identifies next skill: variant analysis, Semgrep hunting, design review, or PoC validation.

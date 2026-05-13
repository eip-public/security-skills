---
name: code-semgrep-hunting
description: Write and run Semgrep rules to find security vulnerabilities in source code. Use when hunting for new vulns in a codebase, building detections for a vuln class, or confirming variant analysis findings.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, semgrep, static-analysis, vuln-hunting, taint-analysis]
    related_skills: []
platforms: [macos, linux]
role: helper
branch_axis: n/a
mcp_required: []
mcp_optional: [semgrep]
tools_required: [semgrep]
tools_optional: [git]
helper_skills: []
tags: [security, semgrep, static-analysis, vuln-hunting, taint-analysis]
---

# Semgrep Vulnerability Hunting

## Methodology note
This skill uses the test-first Semgrep rule workflow common to public Semgrep
and Trail of Bits security skills, adapted to this repository's CVE-routing
style. Keep borrowed methodology small and practical; do not import broad
ruleset catalogs unless the active task needs them.

## Routing doctrine reference
Follow `../ROUTING-DOCTRINE.md` as the house style for:
- branch vs helper ownership
- explicit load triggers
- explicit `When NOT to Use` boundaries where ambiguity exists
- normalized `=== SKILL HANDOFF ===` blocks
- sequential phase loading instead of preloading downstream skills

## When to Use
- Hunting for new vulns in software you're pointed at
- Writing rules to detect a specific vuln class across a codebase
- Building taint-tracking rules for injection/deserialization/SSRF patterns
- Confirming variant analysis findings with a repeatable query

## When NOT to Use
- Merely running existing community rulesets with no custom rule-development need.
- Simple grep-style searches (use ripgrep instead - faster, zero setup)

## Router guidance
This is a tertiary helper skill, not a default phase.

Load it when:
- `code-variant-analysis` has already identified a reusable pattern
- manual review has defined clear sources, sinks, and sanitizers
- a repeatable Semgrep rule will improve confidence or coverage

Do NOT load it merely because a code audit is happening. Manual reasoning should usually come first.

Expected upstream owners:
- `code-variant-analysis`
- `cve-research-analysis` when sources/sinks/sanitizers are already crisply defined
- `cve-wordpress-workflow` only after manual review has identified repeatable WordPress-specific code patterns worth codifying

After this skill completes, hand off based on what the rule run proved:
- back to the owning branch/helper skill if the rule results simply expand coverage and need manual triage or reporting
- to `design-sharp-edges` if repeated findings show an API or configuration footgun rather than isolated code mistakes
- to `cve-poc-validation` only if a rule uncovered a specific reachable instance whose next unanswered question is exploitation evidence

## Standard Handoff Block
When this skill yields, emit:

```
=== SKILL HANDOFF ===
FROM_SKILL:        code-semgrep-hunting
TO_SKILL:          <next skill>
REASON:            <why Semgrep rule development/run is complete enough to hand off>
PRIMARY_ARTIFACTS: <rule yaml, tests, semgrep_results.json, semgrep_findings.md>
STATE_READY:       <what the rule detects, confidence level, confirmed true positives>
OPEN_QUESTIONS:    <what still needs manual triage, design analysis, or PoC validation>
NEXT_TRIGGER:      <what evidence the next skill must produce>
====================
```

## Primary Output
This skill's primary output is tested, repeatable Semgrep rule(s) plus a triaged findings set that can be rerun against the target codebase.

## Minimum Viable Path
The shortest correct use of this skill is:
1. start from an already understood pattern
2. check whether an existing rule already covers the pattern
3. write vulnerable and safe tests first
4. implement the smallest rule that matches the pattern
5. run the tests until they pass cleanly
6. run the rule against the target with `--metrics=off`
7. triage the results manually before calling anything a finding

## Approach Selection

| Scenario | Use |
|----------|-----|
| Data flows from user input to dangerous sink | Taint mode (usually preferred) |
| Dangerous function called regardless of input source | Pattern matching |
| Complex interprocedural / cross-file data flow | Load `code-codeql-analysis`; CodeQL is better suited than Semgrep |
| Quick surface scan | ripgrep first, then Semgrep to confirm |

**Why taint mode is often better:** Pattern matching finds syntax but misses context.
`eval($X)` matches both `eval(user_input)` (vulnerable) and `eval("safe")` (safe).
Taint mode only fires when untrusted data actually reaches the sink.

## Workflow (do not skip steps)

```
[ ] Step 1: Understand the vuln class - what's the source? what's the sink?
[ ] Step 2: Check existing rules before writing a new one
[ ] Step 3: Write test cases FIRST (vulnerable + safe examples)
[ ] Step 4: Inspect AST if pattern matching - semgrep --dump-ast
[ ] Step 5: Write the rule
[ ] Step 6: Run semgrep --test with --metrics=off - iterate until 100% pass
[ ] Step 7: Run against target codebase with --metrics=off
[ ] Step 8: Triage results - true positives to artifacts/semgrep_findings.md
```

## Rule Structure

### Taint Mode (preferred for injection/RCE/deserialization)
```yaml
rules:
  - id: rule-id-here
    languages: [python]  # be specific - avoid 'generic'
    severity: HIGH        # CRITICAL / HIGH / MEDIUM / LOW
    message: "Description of what this finds and why it's dangerous"
    mode: taint
    pattern-sources:
      - pattern: request.args.get(...)
      - pattern: request.form.get(...)
    pattern-sanitizers:
      - pattern: sanitize($X)
    pattern-sinks:
      - pattern: os.system(...)
      - pattern: subprocess.call(...)
```

### Pattern Matching (for structural/syntactic issues)
```yaml
rules:
  - id: rule-id-here
    languages: [java]
    severity: CRITICAL
    message: "Unsafe deserialization - ObjectInputStream with no class filtering"
    patterns:
      - pattern: new ObjectInputStream($STREAM).readObject()
      - pattern-not: |
          $FILTER.accept(...);
          ...
          new ObjectInputStream($STREAM).readObject()
```

## Test File Format (mandatory)
```python
# ruleid: rule-id-here      <- expects a finding
dangerous_sink(user_input)

# ok: rule-id-here          <- expects NO finding
dangerous_sink(sanitized_input)

# ok: rule-id-here
dangerous_sink("hardcoded_safe_value")
```

Run tests: `semgrep --metrics=off --test --config rule-id.yaml rule-id.py`
Must achieve 100% pass rate before running against target.

## Strictness Rules
- One YAML file = one rule. Never combine multiple rules.
- Never use `languages: generic` - always target a specific language.
- Never skip test cases. Untested rules have hidden false positives.
- Taint mode for data flow. Pattern matching for structure only.
- Use `--metrics=off` in every Semgrep command.
- If the rule needs cross-file/interprocedural taint and Semgrep cannot model it reliably, hand off to `code-codeql-analysis` instead of faking precision.
- No `todook` or `todoruleid` annotations.

## Anti-Patterns

Too broad - useless:
```yaml
pattern: $FUNC(...)  # matches everything
```

Too specific - misses variants:
```yaml
pattern: os.system("rm " + $VAR)  # only matches exact string concat
```

Missing sanitizer check - too many FPs:
```yaml
pattern-sinks:
  - pattern: eval(...)  # fires even on eval("safe_literal")
# Fix: add taint mode with sources instead
```

## Running Against a Target Codebase
```bash
# Run your custom rule
semgrep --metrics=off --config path/to/rule.yaml /path/to/target/

# Run with output to file
semgrep --metrics=off --config path/to/rule.yaml --json /path/to/target/ > artifacts/semgrep_results.json

# Optional quick scan with an explicit ruleset if useful for recon.
# Avoid --config auto unless the user asked for registry-backed scanning.
semgrep --metrics=off --config p/security-audit /path/to/target/ --severity ERROR
```

## Triage Output
Save confirmed findings to artifacts/semgrep_findings.md:
- Rule ID and what it detects
- Each true positive: file, line, function, why it's exploitable
- Confidence: High / Medium / Low
- Recommended follow-up: PoC needed? Variant analysis? Patch diff?

## Semgrep Docs (fetch before writing complex rules)
- Rule syntax: https://raw.githubusercontent.com/semgrep/semgrep-docs/refs/heads/main/docs/writing-rules/rule-syntax.md
- Pattern syntax: https://raw.githubusercontent.com/semgrep/semgrep-docs/refs/heads/main/docs/writing-rules/pattern-syntax.mdx
- Taint mode: https://raw.githubusercontent.com/semgrep/semgrep-docs/refs/heads/main/docs/writing-rules/data-flow/taint-mode/overview.md
- Testing rules: https://raw.githubusercontent.com/semgrep/semgrep-docs/refs/heads/main/docs/writing-rules/testing-rules.md

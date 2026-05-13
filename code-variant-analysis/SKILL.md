---
name: code-variant-analysis
description: Hunt for similar vulnerabilities across a codebase after finding an initial bug. Use after identifying a CVE root cause to find other instances of the same pattern using ripgrep, Semgrep, or CodeQL.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, cve, variant-hunting, semgrep, codeql, static-analysis]
    related_skills: [code-codeql-analysis, code-semgrep-hunting, design-sharp-edges]
platforms: [macos, linux]
role: helper
branch_axis: n/a
mcp_required: []
mcp_optional: [semgrep]
tools_required: [git]
tools_optional: [semgrep, codeql]
helper_skills: [code-codeql-analysis, code-semgrep-hunting, design-sharp-edges]
tags: [security, cve, variant-hunting, semgrep, codeql, static-analysis]
---

# Variant Analysis

## Routing doctrine reference
Follow `../ROUTING-DOCTRINE.md` as the house style for:
- branch vs helper ownership
- explicit load triggers
- explicit `When NOT to Use` boundaries where ambiguity exists
- normalized `=== SKILL HANDOFF ===` blocks
- sequential phase loading instead of preloading downstream skills

## When to Use
- A CVE root cause is understood and you want to find other instances
- Hunting for the same bug pattern across different modules or functions
- Building Semgrep/CodeQL queries to systematically detect a vuln class
- Checking if a partial patch left other vulnerable call sites

## When NOT to Use
- Initial vuln discovery in unknown code (use cve-research-analysis first)
- General code review without a known pattern to search for

## Primary Output
This skill's primary output is a triaged list of sibling candidate sites: other locations that may share the same root cause, with confidence and exploitability notes.

## Minimum Viable Path
The shortest correct use of this skill is:
1. state the root cause precisely
2. search for the exact known instance first
3. generalize one element at a time
4. triage new matches manually
5. write findings to `artifacts/variant_analysis.md`

## Trigger threshold
Load this skill only after the root cause is specific enough to search for.

Good triggers:
- repeated missing auth/capability check pattern
- repeated unsafe source-to-sink data flow
- repeated deserialization or parser misuse pattern
- suspicion that a patch fixed one call site but left siblings behind

Bad triggers:
- "maybe there are more bugs somewhere"
- no concrete root-cause statement yet
- purely blackbox targets with no code to search

## Router guidance and handoffs
This is a secondary helper skill that follows root-cause understanding; it should not own initial discovery.

Common upstream owners:
- `cve-research-analysis`
- `cve-wordpress-workflow`
- `code-differential-review`

Load it when one of these is true:
- a known vulnerable instance has been explained precisely enough to generalize
- a patch appears to fix one call site while sibling call sites may remain exposed
- a branch skill needs to know whether the bug is isolated or systemic before moving to PoC or reporting

After this skill completes, hand off based on result quality:
- to `code-semgrep-hunting` only if the pattern is stable enough that a codified Semgrep rule will materially improve coverage or reproducibility
- to `design-sharp-edges` if repeated findings indicate a design-level misuse trap rather than isolated implementation bugs
- back to the owning branch skill if manual variant hunting answered the question without needing codified detection
- to `cve-poc-validation` only when a newly found variant is already reachable and exploitation proof is now the limiting step

## Standard Handoff Block
When this skill yields, emit:

```
=== SKILL HANDOFF ===
FROM_SKILL:        code-variant-analysis
TO_SKILL:          <next skill>
REASON:            <why manual variant hunting is complete enough to hand off>
PRIMARY_ARTIFACTS: <variant_analysis.md, search queries, confirmed match list>
STATE_READY:       <root-cause statement, whether the pattern repeats, confidence of matches>
OPEN_QUESTIONS:    <what still needs codified detection, lab validation, or design analysis>
NEXT_TRIGGER:      <what evidence the next skill must produce>
====================
```

## The Five-Step Process

### Step 1: Nail the Root Cause
Before searching, answer these precisely:
- What is the root cause? (not the symptom - WHY is it vulnerable)
- What conditions are required? (control flow, data flow, state)
- What makes it exploitable? (user input reaches sink? missing check? wrong assumption?)

### Step 2: Exact Match First
Start with a pattern that matches ONLY the known vulnerable instance:
```bash
rg -n "exact_vulnerable_function_or_pattern" .
```
Verify it returns exactly ONE result - the original. If not, your pattern is too broad.

### Step 3: Identify What to Abstract
| Element | Keep Specific | Can Abstract |
|---------|---------------|--------------|
| Function name | If unique to the bug | If the pattern applies to a family |
| Variable names | Never | Always - use wildcards |
| Literal values | If the value itself matters | If any value triggers the bug |
| Arguments | If position matters | Use `...` wildcards |

### Step 4: Generalize One Element at a Time
1. Change ONE thing in the pattern
2. Run and review ALL new matches
3. Classify each: true positive or false positive?
4. If FP rate is acceptable, generalize the next element
5. If FP rate exceeds ~50%, revert and try a different abstraction

### Step 5: Triage Results
For each match document:
- Location: file, line, function
- Confidence: High / Medium / Low
- Exploitability: Is attacker input reachable? Is it controllable?
- Priority: based on impact + exploitability

## Tool Selection

| Scenario | Tool | Why |
|----------|------|-----|
| Quick surface scan | ripgrep | Zero setup, instant |
| Simple pattern matching | Semgrep | Easy syntax, no build needed |
| Data flow (taint tracking) | Semgrep taint mode | Follows values across functions |
| Cross-function / interprocedural | CodeQL | Best for deep call graph analysis |
| Incomplete / unbuildable code | Semgrep | Works without a build |

## Common Pitfalls

### Narrow Search Scope
Bug found in `api/handlers/` → only searching that directory → missing variant in `utils/auth.py`
Always search the ENTIRE repo root.

### Pattern Too Specific
Bug uses `isAuthenticated` → only searching that term → missing `isActive`, `isAdmin`, `isVerified`
Enumerate ALL semantically related attributes and functions.

### Single Vulnerability Class
Original bug is one manifestation → miss others:
- Null equality bypasses (null == null evaluates to true)
- Inverted conditional logic (wrong branch taken)
- Same root cause expressed differently in another language binding
List ALL possible manifestations before searching.

### Missing Edge Cases
Test patterns with: unauthenticated users, null/undefined values, empty collections,
boundary conditions - not just "normal" inputs.

## Semgrep Quick Examples

Simple sink pattern:
```yaml
rules:
  - id: unsafe-deserialization
    languages: [java]
    severity: HIGH
    message: Unsafe deserialization - no class filtering
    pattern: new ObjectInputStream($STREAM).readObject()
```

Taint mode (user input to dangerous sink):
```yaml
rules:
  - id: user-input-to-exec
    languages: [python]
    severity: CRITICAL
    message: User input reaches os.system
    mode: taint
    pattern-sources:
      - pattern: request.args.get(...)
    pattern-sinks:
      - pattern: os.system(...)
```

## Output
Document findings in artifacts/variant_analysis.md:
- Root cause summary
- Search queries used
- Each match: location, confidence, exploitability, verdict
- New CVE candidates (if any confirmed new instances found)

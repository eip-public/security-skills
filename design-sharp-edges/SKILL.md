---
name: design-sharp-edges
description: Identify dangerous API designs, footgun configurations, and insecure defaults that enable security mistakes. Use when auditing a library or framework for misuse-prone patterns during CVE research or new vuln hunting.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, api-design, footguns, insecure-defaults, code-review]
    related_skills: [code-variant-analysis]
platforms: [macos, linux]
role: helper
branch_axis: n/a
mcp_required: []
mcp_optional: []
tools_required: []
tools_optional: []
helper_skills: [code-variant-analysis]
tags: [security, api-design, footguns, insecure-defaults, code-review]
---

# Sharp Edges Analysis

Evaluates whether APIs, configurations, and interfaces are resistant to developer misuse.
Finds designs where the easy path leads to insecurity - these are prime CVE hunting ground.

## Routing doctrine reference
Follow `../ROUTING-DOCTRINE.md` as the house style for:
- branch vs helper ownership
- explicit load triggers
- explicit `When NOT to Use` boundaries where ambiguity exists
- normalized `=== SKILL HANDOFF ===` blocks
- sequential phase loading instead of preloading downstream skills

## When to Use
- Auditing a library or framework for dangerous defaults or footgun APIs
- Researching a CVE to understand the root cause at the design level
- Hunting for new vulns in software you're pointed at
- Explaining WHY a vulnerability exists beyond just WHERE it is

## When NOT to Use
- Straightforward one-off bugs (typo, missing bounds check, single-call mistake) where design analysis adds no routing or explanatory value.
- Before `cve-research-analysis` has identified a root cause — there is nothing design-level to evaluate yet.
- As a substitute for variant hunting: design analysis frames the why; `code-variant-analysis` finds the where.

## Trigger threshold
Load this skill when the vulnerability appears to stem from design pressure, not only local implementation error.

Examples:
- insecure defaults that disable protections silently
- auth/crypto/deserialization APIs that are easy to misuse correctly-looking code with
- permission/capability models where the easy path is insecure
- configuration combinations that create catastrophic failure modes

Skip it for straightforward one-off bugs where design analysis adds no routing or explanatory value.

## Router guidance and handoffs
This is a secondary explanatory/helper skill, not a default branch.

Common upstream owners:
- `cve-research-analysis`
- `cve-memory-windows`
- `cve-wordpress-workflow`
- `code-differential-review`
- `code-variant-analysis`

Load it when one of these is true:
- the patch or root cause suggests developers were set up to fail by the API, config model, or framework defaults
- repeated variants look like the same misuse trap appearing in multiple places
- the final write-up needs to explain why the vulnerability class is likely to recur unless the design changes

Do NOT load it just to make a report sound deeper. There must be a concrete design-level question to answer.

After this skill completes, hand off based on what it established:
- back to the owning branch/helper skill if the main value was root-cause framing for the report or next search step
- to `code-variant-analysis` if the design footgun suggests there may be more code instances to hunt
- to `cve-poc-validation` only if the design analysis has already narrowed the next task to proving a reachable instance

## Standard Handoff Block
When this skill yields, emit:

```
=== SKILL HANDOFF ===
FROM_SKILL:        design-sharp-edges
TO_SKILL:          <next skill>
REASON:            <why design-level analysis is complete enough to hand off>
PRIMARY_ARTIFACTS: <notes on footguns, insecure defaults, API misuse cases>
STATE_READY:       <which design pressure was proven, how it affects exploitability or recurrence>
OPEN_QUESTIONS:    <what still needs code search, lab proof, or reporting>
NEXT_TRIGGER:      <what evidence the next skill must produce>
====================
```

## Core Principle
The pit of success: secure usage should be the path of least resistance.
If developers must read docs carefully or remember special rules to avoid a vuln, the API has failed - and someone probably already got it wrong somewhere.

## Sharp Edge Categories

### 1. Algorithm/Mode Selection Footguns
APIs that let developers choose algorithms invite choosing wrong ones.
Classic example: JWT `alg: none` - attacker controls the header that selects the verification algorithm.

Detection patterns:
- Function parameters named `algorithm`, `mode`, `cipher`, `hash_type`
- Strings/enums selecting cryptographic primitives
- Any untrusted input that influences a security-critical decision

### 2. Dangerous Defaults
Defaults that are insecure, or zero/empty values that disable security.

Questions to ask:
- What happens with `timeout=0`? `max_attempts=0`? `key=""`?
- Does `lifetime=0` mean "never expires" or "always expired"?
- Is the default the MOST secure option available?
- Can any default value disable security entirely?

### 3. Primitive vs Semantic APIs
APIs that expose raw bytes instead of meaningful types invite type confusion.
Same type for keys, nonces, ciphertexts = silent swap bugs.

Detection patterns:
- Functions taking `bytes`/`string` for distinct security concepts
- Parameters that can be swapped without a type error
- Timing-safe vs unsafe comparisons that look identical

### 4. Configuration Cliffs
One wrong setting creates catastrophic failure with no warning.

Detection patterns:
- Boolean flags that disable security entirely (`verify_ssl: false`)
- Magic values (-1 = never expire? infinite retries?)
- Dangerous combinations accepted silently
- Constructor parameters with good defaults but no validation (callers can override)
- Environment variables that override security settings

### 5. Silent Failures
Errors that don't surface, or success that masks failure.

Detection patterns:
- Functions returning bool instead of throwing on security failure
- Empty catch blocks around security operations
- Verification functions that return True on malformed/missing input
- Return values that are easy to ignore

### 6. Stringly-Typed Security
Security-critical values as plain strings enable injection and confusion.

Detection patterns:
- SQL/commands built from string concatenation
- Permissions/roles as arbitrary strings instead of enums
- URLs constructed by joining strings

## Analysis Workflow

### Phase 1: Map the Surface
1. Find all security-relevant APIs: auth, crypto, session, input validation, deserialization
2. Identify every developer choice point: algorithm selection, timeout config, mode flags
3. Find configuration schemas: env vars, config files, constructor params

### Phase 2: Probe Edge Cases
For each choice point:
- Zero/empty/null: what happens with `0`, `""`, `null`, `[]`?
- Negative values: what does `-1` mean?
- Type confusion: can different security concepts be swapped?
- Error paths: what happens on invalid input - silent acceptance or error?

### Phase 3: Three Adversary Models
1. The Scoundrel: actively malicious, controls config. Can they disable security?
2. The Lazy Developer: copy-pastes examples, skips docs. Is the first example secure?
3. The Confused Developer: misunderstands the API. Can they swap params without type errors?

### Phase 4: Validate
- Write minimal code that demonstrates the footgun
- Verify it creates a real, exploitable vulnerability
- Check: is it documented? (docs don't excuse bad design but affect severity)

## Severity Classification

| Severity | Criteria | Examples |
|----------|----------|----------|
| Critical | Default or obvious usage is insecure | `verify=False` default; empty password accepted |
| High | Easy misconfiguration breaks security | Algorithm param accepts "none" |
| Medium | Unusual but possible misconfiguration | Negative timeout has unexpected behavior |
| Low | Requires deliberate misuse | Obscure param combination |

## Checklist Before Finishing
- [ ] Probed all zero/empty/null edge cases
- [ ] Verified defaults are the most secure option
- [ ] Checked for algorithm/mode selection footguns
- [ ] Tested type confusion between security concepts
- [ ] Verified error paths don't silently bypass security
- [ ] Checked constructor params are validated, not just defaulted
- [ ] Considered all three adversary types

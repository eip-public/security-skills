---
name: code-differential-review
description: Security-focused review of patch commits and diffs. Use when analyzing a CVE patch to understand exactly what was fixed, confirm the fix is complete, or find regressions. Adapts depth to change size.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, cve, patch-analysis, code-review, diff]
    related_skills: []
platforms: [macos, linux]
role: helper
branch_axis: n/a
mcp_required: []
mcp_optional: []
tools_required: [git]
tools_optional: [gh]
helper_skills: []
tags: [security, cve, patch-analysis, code-review, diff]
---

# Differential Security Review

## Routing doctrine reference
Follow `../ROUTING-DOCTRINE.md` as the house style for:
- branch vs helper ownership
- explicit load triggers
- explicit `When NOT to Use` boundaries where ambiguity exists
- normalized `=== SKILL HANDOFF ===` blocks
- sequential phase loading instead of preloading downstream skills

## When to Use
- Analyzing a CVE patch commit to understand the exact fix
- Confirming a patch fully closes the vulnerability (not a partial fix)
- Reviewing a PR or commit for new vulnerabilities introduced
- Finding regressions where security code was removed

## When NOT to Use
- Initial CVE triage before a relevant patch, commit range, or changed-file set is known
- Purely blackbox or closed environments where no code diff is available
- Cases where the fix is a one-line, single-file change that is already fully understood and does not affect routing or blast-radius analysis

## Router guidance and handoffs
This is a secondary helper skill, not a default workflow phase.

Load it when:
- `cve-research-analysis` identified one or more patch commits or a multi-file fix that needs structured review
- `cve-wordpress-workflow` found a changelog/patch touching multiple hooks, AJAX actions, REST routes, or capability checks
- `cve-memory-linux` still depends on understanding the exact fix boundary before lab or PoC work can continue

Do NOT load it merely because a repository exists. A concrete patch/diff question must already be present.

After this skill completes, hand off based on what the diff proved:
- to `code-variant-analysis` if the diff reveals a reusable pattern that may exist elsewhere
- to `design-sharp-edges` if the fix is fundamentally about insecure defaults, misuse-prone APIs, or design-level footguns
- back to the owning branch skill (`cve-research-analysis`, `cve-wordpress-workflow`, `cve-memory-linux`, or `cve-memory-windows`) if the main value was understanding the fix boundary
- to `cve-poc-validation` only if the vulnerable path is already reachable and the next unresolved question is exploitation evidence

## Standard Handoff Block
When this skill yields, emit:

```
=== SKILL HANDOFF ===
FROM_SKILL:        code-differential-review
TO_SKILL:          <next skill>
REASON:            <why diff analysis is complete enough to hand off>
PRIMARY_ARTIFACTS: <commit ids, changed files, diff_review.md, key line references>
STATE_READY:       <what the fix added/removed, whether the fix looks partial/full, likely bypass surfaces>
OPEN_QUESTIONS:    <what still needs validation>
NEXT_TRIGGER:      <what evidence the next skill must produce>
====================
```

## Primary Output
This skill's primary output is a fix-boundary assessment: what changed, what security property the patch adds or removes, and whether the fix looks complete, partial, bypassable, or regressive.

## Minimum Viable Path
The shortest correct use of this skill is:
1. inspect the patch or commit range
2. identify the security-relevant changed files and functions
3. determine what guard or assumption changed
4. decide whether the fix boundary looks complete or incomplete
5. write findings to `artifacts/diff_review.md`

## Core Principles

1. Risk-First: Focus on auth, crypto, deserialization, external calls, input validation
2. Evidence-Based: Every finding backed by git history, line numbers, attack scenarios
3. Adaptive: Scale depth to change size (see table below)
4. Honest: State coverage limits and confidence level
5. Always write findings to a file - never just to chat

## Codebase Size Strategy

| Change Size   | Strategy  | Approach |
|---------------|-----------|----------|
| SMALL (<20 files) | DEEP  | Read all deps, full git blame |
| MEDIUM (20-200)   | FOCUSED | 1-hop deps, priority files |
| LARGE (200+)      | SURGICAL | Critical paths only |

## Risk Level Triggers

| Risk Level | Triggers |
|------------|----------|
| HIGH | Auth, crypto, deserialization, external calls, validation removal |
| MEDIUM | Business logic, state changes, new public APIs |
| LOW | Comments, tests, UI, logging |

## Workflow

```
Phase 0: Triage → Phase 1: Code Analysis → Phase 2: Blast Radius
    ↓
Phase 3: Adversarial Modeling → Phase 4: Report
```

### Phase 0: Triage
```bash
git show <commit> --stat          # what files changed
git show <commit> --diff-filter=D # what was DELETED
git log --oneline <prev>..<new>   # all commits in range
```
Classify each changed file as HIGH / MEDIUM / LOW risk.

### Phase 1: Code Analysis
For HIGH risk files:
- Read the full function before and after the change
- git blame removed lines - were they security controls?
- Identify: what check was added? what sink was guarded? what was blocked?
- Ask: does the fix fully guard ALL paths to the vulnerable sink?

### Phase 2: Blast Radius
- Who calls the changed function?
- Are there other callers not covered by the fix?
- Are there other code paths that reach the same sink without the fix?

### Phase 3: Adversarial Modeling
For each HIGH risk change, ask:
- Can an attacker still reach the vulnerable path despite the fix?
- Is there a bypass? A race condition? An edge case not handled?
- Does the fix depend on a config flag an attacker could influence?

### Phase 4: Report
Write to artifacts/diff_review.md:
- Changed files + risk classification
- For each HIGH finding: file, line, what changed, attack scenario, verdict
- Verdict: FULLY FIXED / PARTIAL FIX / BYPASS EXISTS / NEW VULN INTRODUCED

## Red Flags (Investigate Immediately)
- Deleted code from commits named "fix", "security", "CVE"
- Access modifiers relaxed (private → public, internal → external)
- Validation removed without replacement
- Class allowlists added but not enforced on all code paths
- Fix only applied in one branch/version (partial cherry-pick)

## Rationalizations to Reject

| Rationalization | Why It's Wrong |
|-----------------|----------------|
| "Small change, quick look" | Heartbleed was 2 lines - classify by RISK not size |
| "Just a refactor" | Refactors break invariants - treat as HIGH until proven LOW |
| "Fix looks complete" | Check blast radius - other callers may still be exposed |
| "It's documented" | Verify the fix in code, not just in the advisory |

## CVE Patch Analysis Checklist
- [ ] Identified the vulnerable function/sink
- [ ] Found all changed files + classified by risk
- [ ] Read deleted code - was it a security control?
- [ ] Traced all call paths to the sink - all guarded?
- [ ] Checked for partial cherry-picks across branches
- [ ] Verified fix is not bypassable via config or edge case
- [ ] Written findings to artifacts/diff_review.md

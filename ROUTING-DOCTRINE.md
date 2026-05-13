# Security Skill Routing Doctrine

Purpose: keep the CVE/security skill family deterministic, composable, and easy to extend.

## Core Rules

1. One primary owner at a time
- At any point in the workflow, exactly one skill should own the next phase.
- Helper skills may assist, but they do not replace the current branch unless a handoff explicitly says so.

2. Route by situation first, technique second
- Primary branch selection comes before helper/technique selection.
- Example order:
  - `cve-router-triage`
  - `cve-research-analysis`
  - one primary branch skill
  - helper skills only when triggered
  - `cve-poc-validation` last

3. Ecosystem-specific branches beat generic ones
- Prefer the narrowest branch that truly owns the target.
- Example:
  - WordPress target -> `cve-wordpress-workflow`
  - Mobile artifact -> `cve-mobile-android`
  - only otherwise fall back to generic class x OS branches

4. Every skill must define explicit triggers
Avoid vague wording like:
- "use if helpful"
- "consider this skill"
- "can also be used for"

Prefer explicit trigger language like:
- "Load this skill when..."
- "Do NOT load this skill when..."
- "After this skill completes, hand off to..."

5. Every security skill must include a normalized handoff block
Use this exact contract:

```text
=== SKILL HANDOFF ===
FROM_SKILL:        <current skill>
TO_SKILL:          <next skill>
REASON:            <why the current skill is yielding>
PRIMARY_ARTIFACTS: <paths, refs, outputs, evidence>
STATE_READY:       <what is now known or verified>
OPEN_QUESTIONS:    <what remains unresolved>
NEXT_TRIGGER:      <what condition the next skill must satisfy>
====================
```

Exception for terminal skills (role: terminal): `TO_SKILL:` may be omitted, since there is no downstream skill. All other fields remain required.

6. `cve-poc-validation` is terminal-stage only
Do not load PoC skill during:
- triage
- general research
- branch selection
- lab construction

Load it only when:
- a reachable vulnerable path exists, or
- a bounded blackbox validation path is ready.

Acquisition-blocked and structural-blocked cases stop with a clearly labeled
BLOCKED-status report. Do not hand off to `cve-poc-validation` when there is no
reachable target to validate.

7. Helper skills are conditional, never default
Use helper skills only when a concrete question exists.

Examples:
- `code-differential-review`
  - when patch commits or multi-file fix boundaries need structured analysis
- `code-variant-analysis`
  - when root cause is specific enough to generalize
- `code-semgrep-hunting`
  - only after variant analysis or equivalent manual reasoning identifies a stable codifiable pattern
- `code-codeql-analysis`
  - when a deeper interprocedural/source-to-sink query is needed after manual review, Semgrep, or decompiler export
- `design-sharp-edges`
  - when the issue appears design-level, misuse-prone, or default-driven
- `cve-memory-corruption-exploit-dev`
  - when a memory-corruption branch has established a live trigger and the next question is exploit development
- `cve-windows-binary-diff`
  - when Microsoft/Windows patched and vulnerable PE images must be compared to identify the changed functions
- `cve-windows-debug-lab`
  - when a Windows target or driver needs WinDbg/KD/CDB-backed reproduction in the win11-forge lab
- `cve-windows-mitigation-bypass`
  - when a confirmed Windows primitive is blocked by CFG, CET, ASLR, DEP, HVCI, KASLR, SMEP, or pool hardening

8. Cross-branch handoffs are first-class
A branch skill can yield to another branch skill when the workflow changes shape.

Important examples:
- `cve-mobile-android` -> `cve-web-generic`
  - when static APK analysis reveals first-party backend surfaces
- `cve-memory-windows` -> `cve-web-generic`
  - when reversing or traffic analysis of a Windows userland binary reveals a meaningful live backend
- `cve-wordpress-workflow` -> `cve-web-generic`
  - when the next real question is server-side/live endpoint validation rather than more WordPress-local lab work

9. Do not preload large downstream skills
Skills should be loaded sequentially as phases change.
Do not load triage + research + branch + PoC all up front.
This wastes context and weakens routing discipline.

10. Skills should state both positive and negative scope
Every skill should try to include:
- `## When to Use`
- `## When NOT to Use`
- trigger/handoff guidance

11. No stale path references
Every SKILL.md references the doctrine via relative path `../ROUTING-DOCTRINE.md`.
Keep shared CVE lab examples under the canonical `~/exploit-intel/` layout.

12. Resume Workflow
When a user returns to work on an existing CVE lab:
- Inspect the active lab root first (`~/exploit-intel/labs/CVE-YYYY-NNNNN/`) for `INTEL.md`, `report.md`, and `artifacts/poc_run.txt`.
- Resume from the latest concrete workflow state:
  - if `report.md` exists with a terminal verdict, treat the workflow as completed or blocked and summarize/report from there;
  - if `artifacts/poc_run.txt` exists, resume with `cve-poc-validation` only if `report.md` is missing or incomplete;
  - if `INTEL.md` exists, read it and use the `NEXT_BRANCH` field to identify which branch skill to reload.
- Skip `cve-router-triage` and `cve-research-analysis` unless `BLOCKERS` is non-empty, the branch found stale/incomplete research intel, or the user explicitly requests re-triage.
- If no `INTEL.md` exists at the expected path, check for `report.md` first; if it is a BLOCKED report, summarize it instead of restarting.
- If neither `INTEL.md` nor a terminal/blocking `report.md` exists, treat as a new CVE workflow starting at `cve-router-triage`.
- Do NOT re-run the full pipeline from scratch just because a new session has started.

## Canonical Security Flow

```text
cve-identify-candidates
  -> (user picks a CVE)
  -> cve-router-triage
    -> cve-research-analysis
      -> one ecosystem branch (narrow wins):
         - cve-wordpress-workflow
         - cve-mobile-android
         - cve-browser
         - cve-kernel-linux
         - cve-kernel-windows
      -> OR fall through to class x OS fallback:
         - cve-memory-linux
         - cve-memory-windows
         - cve-web-generic       (OS-agnostic)
         - cve-logic-generic     (OS-agnostic)
      -> cve-poc-validation
      -> final reporting
```

Rule: ecosystem branches always beat class x OS fallback. Class x OS branches are loaded only when no ecosystem branch owns the target.

## Handoff Credential Contract

Every handoff that creates or discovers auth material (PATs, API tokens, session cookies, JWT tokens, admin credentials) MUST include a reference to it in the `PRIMARY_ARTIFACTS` field. Downstream skills (especially `cve-poc-validation`) should not have to re-authenticate.

Accepted forms (in order of preference):
1. Environment variable reference: `TARGET_TOKEN` / `TARGET_SESSION` backed by the lab's local `.env` file
2. Local file reference under the lab root, e.g. `.env`, `artifacts/auth-notes.md`, or `lab/seeded-credentials.txt`
3. Instructions to re-create the credential, only when no durable local reference exists

Internal lab-root artifacts, including `report.md`, may preserve exact auth material, tokens, keys, cookies, and secret-shaped values when they are necessary evidence. Do not paste live secrets into chat summaries, public/external writeups, or externally shared handoff text by default. The research phase should create the credential once, and later handoffs should carry the reference to it. `cve-poc-validation` should never have to re-derive auth.

## Frontmatter contract

Every SKILL.md MUST carry this frontmatter. Keys are required even if list values are empty ([]).

```yaml
---
name: <slug matches directory>
description: <one-line, specific>
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [security, cve, ...]
    related_skills: [code-variant-analysis, ...]
platforms: [macos, linux]
role: entry | router | research | branch | helper | terminal
branch_axis: ecosystem | class-os | n/a
mcp_required: [eip-mcp, ...]
mcp_optional: []
tools_required: [docker, git, wp]
tools_optional: [ghidraRun, semgrep]
helper_skills: [code-variant-analysis, ...]
tags: [security, cve, ...]
---
```

All keys shown above are required. Keep top-level `tags` and
`metadata.hermes.tags` aligned; top-level `tags` improves Hermes discovery, and
`metadata.hermes.related_skills` mirrors the loadable helper names used by the
workflow. Use lowercase kebab-case tag values.

Role semantics:
- entry: identifies candidates, pauses for user selection; no inbound handoff
- router: deterministic branch selection; no filesystem work
- research: emits the CVE Research Intel block
- branch: owns one lab-building or target-specific analysis phase; consumes the intel block
- helper: optional secondary skill loaded by a current owner to answer a narrower question; does not replace the current owner unless the handoff explicitly says so
- terminal: cve-poc-validation; no outbound handoff

Branch axis:
- ecosystem: narrow, wins over class-os (wordpress, browser, kernel, mobile)
- class-os: generic fallback (memory, web, logic)
- n/a: entry / router / research / terminal

## CVE Research Intel block

cve-research-analysis MUST emit this before handoff. Branches MUST consume it. Prose block, not YAML.

```
=== CVE RESEARCH INTEL ===
CVE:                 CVE-YYYY-NNNNN
VENDOR:              <string>
PRODUCT:             <string>
ECOSYSTEM:           wordpress | browser | kernel-linux | kernel-windows | mobile-android | none
OS:                  linux | windows | macos | cross-platform
VULN_CLASS:          memory | web | logic | deserialization | injection | xss | auth_bypass | lfi | ssrf | idor | xxe | race | type_confusion | privilege_escalation | information_disclosure
CWE:                 CWE-NNN[, CWE-NNN]
VULNERABLE_VERSIONS: <range or list>
FIXED_VERSION:       <version>
SOURCE_AVAILABLE:    yes | partial | no
SOURCE_URL:          <repo url or n/a>
VULNERABLE_COMMITS:  <sha or range or n/a>
PATCH_COMMITS:       <sha or url or n/a>
PATCH_FILES:         <paths or n/a>
VULNERABLE_FILE:     <path or n/a>
VULNERABLE_FUNC:     <function/method or n/a>
ROOT_CAUSE:          <one-sentence summary>
AUTH_REQUIRED:       yes | no
MIN_ROLE:            <role name or n/a>
EXPLOITATION_PREREQS: <list or none>
BLOCKERS:            <list or none>
KNOWN_POC:           yes | partial | no
REFERENCES:          <advisory url, vendor bulletin, EIP id>
NEXT_BRANCH:         <skill-slug>
==========================
```

Domain branches may emit a superset block (for example `=== WORDPRESS CVE INTEL ===`) that adds ecosystem-specific fields.
Branches treat any `n/a` as a trigger to ask the user, not assume.
`SOURCE_AVAILABLE=partial` means symbols, decompiled code, patch fragments, generated source, or incomplete source are available and useful for analysis, but no complete public source repository is available.
If source availability is `partial`, `SOURCE_URL` should point to the best available source-like artifact or be `n/a` with justification in `BLOCKERS`.

## Work Hygiene (canonical layout)

All work lives under:

```
~/exploit-intel/
├── candidates/                          # cve-identify-candidates output
│   └── YYYY-MM-DD-<kebab-topic>.md
└── labs/
    └── CVE-YYYY-NNNNN/
        ├── README.md
        ├── INTEL.md                     # persisted CVE RESEARCH INTEL block
        ├── lab/
        │   ├── .env                     # COMPOSE_PROJECT_NAME and lab-local env refs
        │   ├── Dockerfile
        │   ├── docker-compose.yml
        │   ├── seed.sh
        │   └── assets/
        ├── poc/
        │   └── exploit.<ext>
        ├── artifacts/
        │   ├── request.http
        │   ├── response.http
        │   ├── shell-output.txt
        │   └── screenshots/             # required when impact is visual, skip otherwise
        ├── report.md
        └── tmp/                         # deleted at branch -> PoC handoff
            └── repo/
```

Hard rules:
- Never create files in ~/ or anywhere above ~/exploit-intel/.
- One lab per CVE.
- Source checkouts may exist under `tmp/` during active research.
- No `.git/` directories may remain under a lab after branch -> PoC handoff or finalization.
- No ad-hoc dirs at any level.
- `tmp/` is scratch; delete it at branch -> PoC handoff, including any `.git/` directories or build caches.

## Docker lab naming

Docker-owning branches build the lab, but all branches follow the same names.

For every Docker-backed lab, create `lab/.env` with:

```env
COMPOSE_PROJECT_NAME=cve-yyyy-nnnnn-lab
```

Use the lowercase CVE slug for Docker objects: `cve-yyyy-nnnnn`, derived from `CVE-YYYY-NNNNN`.

Compose rules:
- Do not use fixed `container_name`; let Compose derive containers from `COMPOSE_PROJECT_NAME`.
- Use short semantic service names: `web`, `smtp`, `db`, `redis`, `mail`, `callback`, `target`, `worker`.
- Explicitly name every locally built image.
- Single locally built target: `image: cve-yyyy-nnnnn-lab:local`.
- Multiple locally built targets: `image: cve-yyyy-nnnnn-web:local`, `image: cve-yyyy-nnnnn-smtp:local`, `image: cve-yyyy-nnnnn-worker:local`, etc.
- Third-party base images such as `postgres`, `mysql`, `redis`, or `wordpress` do not need renaming unless the lab builds a custom wrapper image.

Example:

```yaml
services:
  web:
    build: .
    image: cve-yyyy-nnnnn-web:local
    ports:
      - "8080:8080"

  smtp:
    build: ./smtp
    image: cve-yyyy-nnnnn-smtp:local
```

## External methodology references

When a skill adopts a concrete workflow pattern from a public security skill, note the source near
the relevant section in that skill. Keep attribution small and specific; do not add broad credits
for generic security knowledge or expand skills with copied reference material.

Artifacts vs tmp:
- artifacts/ is evidence of impact. Small, reviewable. Screenshots required when impact is visual.
- Debug-only files (pcaps, tcpdump, strace logs, heap dumps) stay in tmp/ and die at handoff. Exception: pcap IS the evidence for protocol-level vulns (rare).

Naming (strict):
- CVE dir: CVE-YYYY-NNNNN (uppercase)
- Candidate list: YYYY-MM-DD-<kebab-topic>.md
- PoC: exploit.<ext> only
- Report: report.md only
- Intel file: INTEL.md (uppercase)

## Authoring Checklist for New Security Skills

Before considering a new skill complete, verify:
- [ ] directory slug matches frontmatter `name:`
- [ ] skill has a narrow, specific ownership statement
- [ ] skill includes `When to Use`
- [ ] skill includes `When NOT to Use` if ambiguity is likely
- [ ] skill defines explicit load triggers
- [ ] skill defines downstream handoff conditions
- [ ] skill includes the normalized `=== SKILL HANDOFF ===` block
- [ ] any referenced sibling skills actually exist under canonical names
- [ ] branch-vs-helper role is explicit
- [ ] terminal skills are clearly marked terminal

## Anti-Patterns to Avoid
- Router skill that tries to also do the entire workflow
- Helper skill that reads like a default phase
- Branch skill that never says when to yield
- Semgrep/static-analysis skill loaded before the bug pattern is understood
- PoC skill loaded before reachability/prerequisites are established
- Mobile/static branch claiming server-side exposure without a handoff to live validation
- `cve-web-generic` branch used as a substitute for source/patch analysis when source is available and that is the real next step

## BLOCKED Types

Two distinct BLOCKED states exist. Distinguish them explicitly in every report.

**Acquisition BLOCKED**: No installer, no source, no hosted access obtainable.
- Terminal output is a BLOCKED-status `report.md` that explains what was tried, why it failed, and what would unblock it (specific installer, vendor trial, disclosure date, hosted lab).
- Do NOT hand off to `cve-poc-validation`.
- Speculative analysis is allowed only when labeled `[UNTESTED]` throughout.

**Structural BLOCKED**: No skill exists for this target type (macOS kernel/XNU, iOS, firmware/embedded).
- Terminal output is a BLOCKED-status `report.md` labeled `[SKILL GAP]`.
- Required fields: which skill is missing, what methodology gap it creates, what existing skill was used as the closest approximation (if any), and what adaptation caveats apply.
- Include a one-line recommendation to author the missing skill before the next occurrence.
- Do NOT silently shoehorn the work into an adjacent skill's methodology without documenting the adaptation.

Current known Structural BLOCKs:
- macOS kernel / XNU → no `cve-kernel-macos` skill; adapt `cve-memory-linux` for userland, document as `[SKILL GAP]` for ring-0.
- iOS → no `cve-mobile-ios` skill; document as `[SKILL GAP]`.
- Firmware / embedded → no dedicated skill; document as `[SKILL GAP]`.

## Short Rule of Thumb
- Situation chooses the branch.
- Findings choose the helper.
- Evidence chooses the handoff.
- `cve-poc-validation` comes last.

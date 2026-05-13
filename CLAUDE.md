# Working in security-skills

Project-level notes for anyone — human or AI — making changes here.
This file is the source of truth for the conventions the rest of the
repo holds itself to. Read it first, then read `ROUTING-DOCTRINE.md`.

## What this repo is

A Hermes Agent skill pack for defensive vulnerability research. The
content is prose, not code: each `SKILL.md` tells an autonomous agent
when to load, what evidence it needs, which tools it expects, how it
hands off to the next phase, and what output a user should receive.

The repo is markdown-only. The surrounding host toolchain is installed
by [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes),
not from this repo.

## Where things live

```
ROUTING-DOCTRINE.md                source of truth for routing rules
README.md                          user-facing intro + skill index
SECURITY.md                        threat-model posture for the host
CONTRIBUTING.md                    add-a-skill walkthrough
CLAUDE.md                          this file

cve-*/SKILL.md                     CVE workflow skills (router, research,
                                   branch owners, terminal stages)
code-*/SKILL.md                    code-analysis skills (diff review,
                                   variant analysis, semgrep, codeql)
design-*/SKILL.md                  design-time review (sharp-edges)

<skill>/references/<topic>.md      durable reference material consumed
                                   by the skill (rare; most skills do
                                   not have one)

security-skill-routing-graph.html  visualization of the routing tree
```

## Hard rules

These are the invariants the routing doctrine and existing skills rely
on. Don't break them.

1. **`ROUTING-DOCTRINE.md` is canonical for routing.** When a skill's
   procedure conflicts with the doctrine, the doctrine wins. Don't
   restate doctrine rules inside skill bodies; reference the doctrine
   and write the skill-specific specifics.

2. **One primary owner at a time.** At any point in the workflow,
   exactly one skill owns the next phase. Helper skills assist but
   don't replace the current branch unless a handoff explicitly says
   so. Preserve this in any new skill's `When to Use` / `When NOT to
   Use` sections.

3. **Frontmatter is a contract, not decoration.** Every `SKILL.md`
   carries the standardized YAML frontmatter — `name`, `description`
   (a one-sentence trigger), `version`, `author`, `license`,
   `metadata.hermes.tags`, `platforms`, `role`, `branch_axis`,
   `mcp_required`, `mcp_optional`, `tools_required`, `tools_optional`,
   `helper_skills`, `tags`. Missing or freeform fields break routing
   assumptions.

4. **Use the normalized handoff block.** Skills that complete a phase
   emit `=== SKILL HANDOFF ===` (or the routing-doctrine-defined
   equivalent for that phase, e.g. `=== CVE RESEARCH INTEL ===`).
   Don't invent new handoff shapes.

5. **Canonical lab layout.** Skills work under
   `~/exploit-intel/labs/CVE-YYYY-NNNNN/`. The standard files are:

   ```
   INTEL.md          persisted CVE RESEARCH INTEL block; resume point
   report.md         final report (terminal stage only)
   exploit.<ext>     PoC artifact
   artifacts/        evidence of impact (small, reviewable)
   tmp/              scratch; deleted at branch -> PoC handoff
   ```

   `INTEL.md` is the resume contract. Reading it must be enough for a
   new session to continue from the recorded `NEXT_BRANCH`. Don't
   spread state across other files.

6. **Skill prose is for the agent.** Terse, imperative. Explicit
   `Load this skill when...` and `Do NOT load this skill when...`
   triggers — no "use if helpful" / "can also be used for" / "you may
   want to". The frontmatter `description` is a trigger, not a
   summary.

7. **Documented skill gaps are intentional.** The routing doctrine
   names `cve-kernel-macos`, `cve-mobile-ios`, and firmware/embedded
   as Structural BLOCKs — skills that intentionally don't exist. When
   a CVE falls into one of these, the workflow emits a `[SKILL GAP]`
   marker; it does not invent a substitute. Don't add a stub skill
   for these without an explicit design discussion.

8. **No machine-specific paths in skill prose.** Use `$HOME` or the
   canonical `~/exploit-intel/labs/...`. Never bake a username or a
   real hostname into a skill.

## Validation before commit

The repo has no validator script. Manual checklist:

- Frontmatter matches a sibling SKILL.md field-for-field.
- Positive and negative triggers are explicit.
- Cross-references to other skills use relative paths that resolve
  from the skill's own directory (`../<sibling>/SKILL.md`).
- New skills are added to `README.md` and, if they change routing, to
  `ROUTING-DOCTRINE.md`.
- New tool dependencies are added to the canonical host installer in
  [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes)
  (`installers/hermes/components/`), not here.

## Adding a new skill

See `CONTRIBUTING.md` for the full walkthrough. The short version:

1. Pick a directory name matching the routing axis (`cve-*`, `code-*`,
   `design-*`).
2. Copy the frontmatter from a sibling skill, fill in `name`,
   `description`, `role`, `branch_axis`.
3. Write the body. Reference `../ROUTING-DOCTRINE.md` for house rules
   instead of restating them. Define explicit triggers and a handoff
   block.
4. If the skill ships a new tool dependency, add it to the canonical
   host installer in `eip-public/eip-hermes`.
5. Update `README.md` (and `ROUTING-DOCTRINE.md` if routing changes).

## Style

- Skill prose: terse, imperative, agent-facing. Not user-facing
  documentation.
- Comment the *why*, not the *what*. SKILL.md content should be
  self-evident from the procedure; reserve narrative for non-obvious
  constraints.
- No session-talk language ("session learning", "we found", "today's
  run", "this session"). Skills are durable rules, not journal
  entries.
- No first-person, no personal narration, no references to individual
  contributors. The skill speaks to the agent.

## Documentation hierarchy

When updating docs:

- `README.md` — high-level intro, who it's for, skill index, lab
  layout, install pointer. User-facing.
- `ROUTING-DOCTRINE.md` — routing rules, ownership, handoff contract,
  canonical artifact rules. Agent- and contributor-facing.
- `SECURITY.md` — threat model and posture for the host the skills
  run on.
- `CONTRIBUTING.md` — contributor flow.
- `CLAUDE.md` (this file) — invariants the repo holds itself to.

If a fact lives in two places, the two will drift. Pick the right
home and link from the other.

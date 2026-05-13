# Contributing to security-skills

Thanks for the interest. This repo is a focused skill pack for the
Hermes Agent: deterministic CVE-triage workflows, code-analysis skills,
and the doctrine that keeps them composable. Most contributions fit one
of three shapes.

## Adding a new skill

1. Pick a directory name that matches the routing doctrine -- `cve-*`
   for CVE branch or stage skills, `code-*` for code-analysis skills,
   `design-*` for design-time review skills.
2. Create `<your-skill>/SKILL.md` with frontmatter that mirrors the
   contract used by the existing skills:

   ```yaml
   ---
   name: <skill-name>
   description: <one-sentence "when to load this" trigger>
   version: 1.0.0
   author: Hermes Agent
   license: MIT
   metadata:
     hermes:
       tags: [security, ...]
       related_skills: []
   platforms: [macos, linux]
   role: <branch|helper|router|stage>
   branch_axis: <ecosystem|class-os|n/a>
   mcp_required: []
   mcp_optional: []
   tools_required: []
   tools_optional: []
   helper_skills: []
   tags: [security, ...]
   ---
   ```

3. Reference `../ROUTING-DOCTRINE.md` from the body so the file inherits
   the house rules instead of restating them.
4. Define explicit `Load this skill when...` and `Do NOT load this skill
   when...` triggers. Vague wording ("use if helpful", "can also be
   used for") is rejected by review.
5. Use the normalized `=== SKILL HANDOFF ===` block when the skill
   completes a phase. See any existing branch skill for the exact shape.
6. Add the new skill to the relevant section of `README.md` and, if it
   changes routing, to `ROUTING-DOCTRINE.md`.
7. If the skill ships a new tool dependency, add it to the canonical
   host installer in [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes)
   (under `installers/hermes/components/`) — not here. This repo no
   longer ships its own toolchain installer.

## Fixing or extending an existing skill

- Keep frontmatter standardized and complete -- missing fields break
  routing assumptions.
- Don't introduce a new branch axis without updating
  `ROUTING-DOCTRINE.md`. Ecosystem-first beats class-x-OS; helpers do
  not replace branch owners.
- Don't reference retired skills, removed paths, or out-of-tree tool
  names in skill prose. If a tool moved, fix the reference here.
- Don't leak machine-specific paths (`/Users/<name>/...`, `/home/<name>/...`).
  Use `$HOME` or document the canonical lab path `~/exploit-intel/labs/...`.

## Style

- Skill prose is for the agent, not the reader -- be terse and
  imperative.
- Comment the *why*, not the *what*. Skill steps should be self-evident;
  reserve commentary for non-obvious constraints (e.g. why a check is
  needed, what guarantee a handoff block carries).
- Frontmatter `description` is a one-sentence trigger, not a summary.

## Local checks before opening a PR

Skim the diff for:

- frontmatter completeness (use any sibling SKILL.md as a checklist);
- explicit positive and negative `When to use` triggers;
- normalized `=== SKILL HANDOFF ===` blocks where the skill owns a phase;
- no `/Users/...` or `/home/<name>/...` paths;
- updated cross-references in `README.md` and `ROUTING-DOCTRINE.md`.

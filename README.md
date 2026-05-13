# Security Skills

Security Skills is a Hermes Agent skill pack for defensive vulnerability research, CVE triage, exploit-intelligence workflows, code review, and security-tool-assisted analysis.

The project turns security work into repeatable, reviewable workflows. Instead of relying on ad hoc prompts, each `SKILL.md` defines when it should be loaded, what evidence it needs, which tools it expects, how it hands off to the next phase, and what output a user should receive.

## Who this is for

Use this repository if you want Hermes Agent to help with:

- triaging CVEs and choosing the right analysis path;
- researching advisories, patches, public exploit signals, and affected products;
- building reproducible local labs for authorized validation;
- reviewing security patches and hunting variants;
- writing and running Semgrep or CodeQL analysis;
- analyzing browser, kernel, WordPress, Android, Windows, Linux, web, logic, and memory-corruption vulnerabilities;
- standardizing security research notes, handoffs, and final reports.

These skills are intended for authorized defensive research, internal security review, and lab-based vulnerability validation. Before pointing the agent at anything beyond a dedicated research host, read [`SECURITY.md`](SECURITY.md) — the skills assume broad local privilege and the per-CVE lab directories are expected to contain working PoC code.

## What is in this repository

### Core doctrine

- `ROUTING-DOCTRINE.md` — the source of truth for how the security skill family routes work, hands off between phases, and keeps skill behavior consistent.

### CVE workflow skills

The CVE workflow is designed as a deterministic pipeline:

```text
cve-identify-candidates
  -> cve-router-triage
    -> cve-research-analysis
      -> one target-specific branch skill
        -> cve-poc-validation
```

Primary CVE skills:

- `cve-identify-candidates` — turns a research intent into ranked CVE candidates.
- `cve-router-triage` — routes a named CVE to the right downstream skill.
- `cve-research-analysis` — builds the standardized CVE research dossier.
- `cve-poc-validation` — terminal-stage lab validation and reporting.

Target-specific branch skills:

- `cve-browser`
- `cve-kernel-linux`
- `cve-kernel-windows`
- `cve-logic-generic`
- `cve-memory-linux`
- `cve-memory-windows`
- `cve-mobile-android`
- `cve-web-generic`
- `cve-wordpress-workflow`

Exploit-development and Windows helper skills:

- `cve-memory-corruption-exploit-dev`
- `cve-windows-binary-diff`
- `cve-windows-debug-lab`
- `cve-windows-mitigation-bypass`

### Code-analysis and security-review skills

- `code-differential-review` — structured security review of patch commits and diffs.
- `code-variant-analysis` — hunt for similar bugs after one root cause is known.
- `code-semgrep-hunting` — write and run Semgrep rules for a known pattern.
- `code-codeql-analysis` — build and query CodeQL databases for deeper source-to-sink analysis.
- `design-sharp-edges` — identify dangerous defaults, footguns, and misuse-prone APIs.

### Host toolchain

The skills assume the surrounding security toolchain (Semgrep, EIP MCP, Ghidra/BinExport, CodeQL, Docker, Burp, Metasploit, Wine, Android analysis tools, etc.) is already installed on the host. The canonical installer is [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes), which provisions Hermes Agent + Honcho memory + local Ollama + this skill pack + the toolchain in one pass.

## How the skills work

Each skill is a self-contained `SKILL.md` with:

- frontmatter metadata for routing, tools, MCP dependencies, tags, and helper skills;
- clear “when to use” and “when not to use” guidance;
- prerequisite checks and required evidence;
- a step-by-step procedure;
- standardized handoff blocks for multi-stage work;
- final reporting expectations.

The important operating rule is: one skill owns the current phase. Helper skills can answer narrow questions, but they do not replace the current owner unless the handoff explicitly says so.

## Typical workflows

### Start from a research topic

Use `cve-identify-candidates` when you have a topic, vendor, product family, CWE, or time window but have not selected a CVE yet. It ranks candidates and pauses for user selection.

### Start from a known CVE

Use `cve-router-triage` first. It classifies the CVE and names the next skills to load.

### Review a security patch

Use `code-differential-review` when you have a patch commit, PR, advisory diff, or multi-file fix boundary to analyze.

### Hunt for variants

Use `code-variant-analysis` after a concrete root cause is known. If the pattern can be codified, hand off to `code-semgrep-hunting` or `code-codeql-analysis`.

### Validate a PoC

Use `cve-poc-validation` only after a branch skill has prepared a reachable target or bounded blackbox validation path. Acquisition-blocked and structurally blocked cases stop with a BLOCKED report instead of forcing PoC validation.

## Recommended lab layout

The CVE skills use the canonical lab layout:

```text
~/exploit-intel/
  labs/
    CVE-YYYY-NNNNN/
      INTEL.md
      report.md
      exploit.<ext>
```

`INTEL.md` is the resume point for future sessions. When returning to an existing lab, read it first and continue from the `NEXT_BRANCH` field instead of restarting the pipeline.

## Using these skills with Hermes Agent

If you installed Hermes via [`eip-hermes`](https://github.com/eip-public/eip-hermes), this skill pack is already cloned to `~/.hermes/skills/eip-cve` and wired up — there is nothing more to do here.

For a manual setup where Hermes is installed by some other path:

```bash
git clone https://github.com/eip-public/security-skills.git
cd security-skills

mkdir -p ~/.hermes/skills/eip-cve
for skill in */SKILL.md; do
  dir="${skill%/SKILL.md}"
  ln -sfn "$PWD/$dir" "$HOME/.hermes/skills/eip-cve/$dir"
done
```

Then start a new Hermes session so the skill index is refreshed.

## Repository structure

```text
.
├── ROUTING-DOCTRINE.md
├── README.md
├── code-*/SKILL.md
├── cve-*/SKILL.md
├── design-sharp-edges/SKILL.md
└── security-skill-routing-graph.html
```

## Maintenance expectations

When adding or editing a skill:

- keep `ROUTING-DOCTRINE.md` current;
- keep frontmatter complete and standardized;
- define positive and negative scope;
- include explicit load triggers;
- use normalized handoff blocks;
- avoid stale tool names, paths, or retired workflow branches;
- verify with `git diff --check` before committing.

## Status

This repository is a working security skill pack. The current focus is standardization, reviewability, and reliable handoffs across the CVE and code-analysis workflow family.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for how to add a new skill, the frontmatter contract each skill must satisfy, and the routing-doctrine rules contributions are expected to follow.

## License

[MIT](LICENSE).

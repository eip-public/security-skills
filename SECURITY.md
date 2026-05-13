# Security model

security-skills is a Hermes Agent skill pack for defensive
vulnerability research. The skills themselves are prose, but they
drive an autonomous agent to do offensive work (build labs, run PoCs,
diff binaries, query CodeQL/Semgrep). Read this before pointing the
agent at anything beyond a dedicated research host.

## What the skills assume about the host

- The skills assume a Linux host running Hermes Agent (or an
  equivalent runner) with broad local privilege. They don't
  themselves grant any privilege, but they expect that:
  - The agent can read and write under `$HOME` freely.
  - The agent has access to the host security toolchain (Semgrep,
    Ghidra/BinExport, CodeQL, Docker, Burp, Metasploit, etc.). The
    canonical installer is [`eip-public/eip-hermes`](https://github.com/eip-public/eip-hermes);
    `SECURITY.md` in that repo documents what its installer grants.
  - The agent can run arbitrary binaries — including unverified PoC
    code in the per-CVE lab directories.
- Lab directories under `~/exploit-intel/labs/<CVE>/` are expected to
  contain working PoC code, debugger artifacts, and reverse-engineered
  binaries. **Assume their contents are hostile.** Don't run them on
  hosts that hold production data, credentials, or anything you wouldn't
  willingly hand to a stranger.

## What this repo does and doesn't install

This repo is markdown-only. It does not install packages, services,
or system tools. The skills *invoke* tools; the tools themselves come
from `eip-hermes` (or whatever installer you used to set up the host).

If a skill assumes a tool that isn't present, the agent is expected
to surface that gap rather than silently install it.

## Recommended posture

- Run on a dedicated research host or VM. Treat the host the way you
  would treat any "I'm going to fuzz random binaries on this box"
  machine.
- Don't share the host with production data or services. The agent
  has no isolation from the rest of the host — if it executes a
  malicious PoC, that PoC runs as your user.
- Firewall the host. The skills don't open listeners themselves, but
  the tools they invoke might (a Docker container with a published
  port, a debugger MCP, a vulnerable service used as a lab target).
- Network-segment any lab targets you reach from these skills.
  Authorized research on a third-party system requires written
  authorization; the skills enforce no such check.

## Reporting an issue

There is no private vulnerability-disclosure channel set up for this
project today. If you find a skill that gives unsafe instructions, an
install-script behavior that exceeds what's documented above, or a
sensitive leak in a SKILL.md, open a GitHub issue. Please don't
include real target names, host details, or unredacted PoC payloads
in a public issue.

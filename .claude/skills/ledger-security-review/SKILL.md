---
name: ledger-security-review
description: "Full-depth security audit methodology for any Ledger embedded application (C or Rust) — APDU reachability, trust model, memory safety, cryptographic misuse, clear-signing bypass. Use when the user asks to audit, pentest, find vulnerabilities, security review an embedded Ledger app, or mentions APDU fuzzing, exploit PoC, clear-signing bypass, or embedded firmware security."
---

# Ledger Embedded Application Security Review

The full methodology lives in the `ledger-app-ai-instructions` submodule so it stays in sync
with the upstream repo.

**Read it now, before doing anything else:**
`ledger-app-ai-instructions/skills/SECURITY.REVIEW.md`

Then follow it verbatim, tracking the Phase 0–10 task list it defines.

If that file is missing, the submodule has not been initialized — run
`git submodule update --init --recursive` from the repo root and read it again.

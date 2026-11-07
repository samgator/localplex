## Description

Describe your changes, the rationale behind them, and link relevant issues.

## Non-Negotiables Checklist

- [ ] **Performance Budgets Verified:**
  - [ ] Cold start interactive library under 500 ms (`python3 tools/bench/cold_start.py`)
  - [ ] Idle memory under 150 MB resident (`python3 tools/bench/memory_bench.py`)
  - [ ] Playback start under 300 ms and instant seek (`python3 tools/bench/playback_bench.py`)
- [ ] **Design & Craft Discipline:**
  - [ ] Adheres strictly to `docs/DESIGN.md` (no generic AI tropes, correct focus rings, proper motion tokens)
  - [ ] UI copy is terse and human-voiced
- [ ] **Rule 4 Compliance:**
  - [ ] Never alters, renames, moves, or touches media files or folders on the user's disk
- [ ] **Privacy & Security:**
  - [ ] No personal emails (use GitHub noreply: `username@users.noreply.github.com`)
  - [ ] No local home directory paths or secrets in code, logs, or commits
  - [ ] Ran `./tools/git/pre-commit.sh` successfully
- [ ] **Testing & Code Quality:**
  - [ ] All unit tests pass (`ctest --test-dir build --output-on-failure`)
  - [ ] Every new source file has SPDX identifier `// SPDX-License-Identifier: GPL-3.0-or-later`

# Contributing to localplex

Thank you for your interest in contributing to localplex. localplex is a fast, offline desktop video library and player built with Qt 6 (C++20 and Qt Quick) and libmpv.

## Core Tenets

Before submitting code or proposing changes, please keep these non-negotiable principles in mind:

1. **Performance is the product.** We measure cold start times, idle memory, and UI framerates against strict performance budgets. Changes that degrade startup or frame pacing will not be merged.
2. **Design discipline.** The UI avoids generic tropes (no drop shadows on every element, no glassmorphism, no emoji icons, no stock rounded dashboards). Follow `docs/DESIGN.md`.
3. **Never touch the user's media.** We never rename, move, write metadata tags to, or modify user media directories.
4. **Offline and private by default.** No telemetry, no user tracking, and no unsolicited network queries.
5. **Privacy of contributors and users.** Never commit real personal filenames, home paths, personal email addresses, or secrets.

## Development Workflow

### Prerequisites

- CMake 3.22+
- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2019+)
- Qt 6.5+ (Core, Gui, Quick, Qml, QuickControls2, OpenGL, Sql)
- libmpv development libraries (0.35+)
- SQLite 3.35+

### Building

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Run the executable:

```bash
./build/localplex
```

### Git and Commit Hygiene

- **Public identity:** Configure git to use your public handle and a GitHub noreply address (`username@users.noreply.github.com`). Commits containing private email addresses or personal identifying data will be rejected by pre-commit checks.
- **Commit format:** Use Conventional Commits in the imperative mood (`feat: add chapter markers to timeline`, `fix: prevent audio desync on rapid seek`).
- **No secrets or private paths:** Ensure no absolute user paths (e.g. `~` expansions or local directory roots) or private media paths enter the repository.
- **Pre-commit hooks:** Run `tools/git/pre-commit.sh` or configure hooks using `git config core.hooksPath .githooks`.

### Licensing

All contributions are licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later). Every new source file must include the SPDX identifier header:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
```

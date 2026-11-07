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

#### Linux (Debian / Ubuntu / Fedora / Arch)

```bash
# Debian / Ubuntu:
sudo apt-get install -y cmake ninja-build build-essential libmpv-dev libsqlite3-dev \
  qt6-base-dev qt6-declarative-dev qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-window

# Arch Linux:
sudo pacman -S --needed cmake ninja base-devel mpv sqlite qt6-base qt6-declarative qt6-quickcontrols2

# Build & Run:
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/localplex
```

#### macOS (Apple Silicon / Intel)

```bash
brew install cmake ninja mpv sqlite qt@6
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build -j$(sysctl -n hw.ncpu)
./build/localplex
```

#### Windows 10 / 11 (MSVC 2019+)

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:\Qt\6.6.3\msvc2019_64;C:\mpv-dev"
cmake --build build --config Debug
.\build\Debug\localplex.exe
```

### Running Tests and Performance Budgets

Every change must pass the automated test suite and conform to performance budgets:

```bash
# Run unit tests:
ctest --test-dir build --output-on-failure

# Verify performance budgets:
python3 tools/bench/cold_start.py      # Cold start < 500 ms
python3 tools/bench/memory_bench.py    # Idle RSS < 150 MB
python3 tools/bench/playback_bench.py  # Playback start < 300 ms, instant seek
python3 tools/bench/library_bench.py   # 5k-item catalog < 15 ms
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

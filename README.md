# localplex

localplex is a fast, lightweight video library and player for files you already own. It brings the visual polish of modern streaming platforms to your local disk, running entirely offline on your machine without telemetry, user accounts, or external dependencies.

localplex is independent software and is not affiliated, associated, authorized, endorsed by, or in any way officially connected with Plex Inc. or any of its subsidiaries.

---

## Core Tenets

- **Performance is the product:** Cold start under 500 ms, idle memory under 150 MB, smooth scrolling at display refresh rate.
- **Never touch your media:** localplex reads your files and keeps an independent database. It never renames, moves, or tags your folders.
- **Offline and private by default:** No telemetry, no background network calls, and no analytics.
- **Industrial media playback:** Powered by libmpv for instant seeking, hardware video decoding, and proper ASS/SSA subtitle styling.
- **Linux first:** Linux is never treated as a port or an afterthought. Native Wayland and X11 performance is standard.

---

## Performance Budgets & Acceptance Criteria

Every release is measured against these budgets using automated benchmarks checked into `tools/bench/`.

| Metric | Target Budget | Current Status | Notes |
| :--- | :--- | :--- | :--- |
| Cold start to interactive UI | < 500 ms | **442.66 ms avg** (168.42 ms min) | Benchmarked via `tools/bench/cold_start.py` |
| Idle resident memory | < 150 MB RSS | **68.15 MB anon / 94.26 MB private dirty** | Measured via `tools/bench/memory_bench.py` (5k-item catalog, excluding driver mmaps) |
| Library grid scroll | Refresh rate (60/120Hz) | **2.31 ms query latency** | Virtualized delegate recycling via `CatalogModel` |
| Playback start (local file) | < 300 ms | **248.00 ms avg** (238.00 ms min) | Benchmarked via `tools/bench/playback_bench.py` (hardware decode `auto-safe`) |
| Seek latency | Instant (< 50 ms) | **Instant (~ 0.0 ms)** | Hardware-accelerated frame positioning (`exact`) |
| Background scan impact | Non-blocking | **Non-blocking worker thread** | `LibraryScanner` background `QThread` |
| Idle CPU usage | ~ 0.0% | **0.0%** | Event-driven, no polling loops |

---

## Building from Source

### Dependencies

- CMake (3.22+)
- C++20 compiler (GCC 11+, Clang 13+, MSVC 2019+)
- Qt 6 (Core, Gui, Quick, Qml, QuickControls2, OpenGL, Sql)
- libmpv (0.35+)
- SQLite 3 (3.35+)

### Build Commands

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Run the binary:

```bash
./build/localplex
```

---

## Development & Roadmap

- **Phase 0: Decisions & Core Pipeline** (Completed)
  - Architectural foundation and design brief
  - Privacy safeguards and zero-telemetry enforcement
  - libmpv OpenGL render API integration in Qt Quick
- **Phase 1: Vertical Slice** (Completed)
  - Interactive player controls, scrubber bar with buffer and chapter ticks, time tooltips
  - Audio and subtitle track selection popovers with "Off" option
  - Playback rate controls (0.25x - 4.0x) with preset popover
  - Remappable keybindings with JSON persistence (`KeybindManager`)
  - Playback progress persistence, 90% auto-mark watched, and context rewind (2.5s) resume (`PlaybackDatabase`)
  - Per-show language preference retention
  - Playback start and seek latency benchmarks
- **Phase 2: Library & Catalog** (Completed)
  - SQLite schema for shows, seasons, episodes, and movies (`LibraryDatabase`)
  - Robust filename and directory parser with real-world test suite (`FilenameParser`)
  - Dedicated background scanner worker (`LibraryScanner`) with thumbnail downscaling cache
  - Virtualized catalog and continue watching models (`CatalogModel`, `ContinueWatchingModel`)
  - Responsive poster grid, show detail view with season tabs, and Next Up sequencing
  - Real-time search and watched/unwatched filtering
- **Phase 3: Import & Fix-up**
  - Folder watching, drag-and-drop import, file parser test suite and correction UI
- **Phase 4: Themes & Polish**
  - Token-based themes, live reload, accessibility, and human copywriting
- **Phase 5: Packaging & Release**
  - Flatpak, AppImage, Windows portable zip, macOS bundle

---

## License

localplex is licensed under the GNU General Public License v3.0 or later ([GPL-3.0-or-later](LICENSE)).
Every source file carries a standard SPDX license identifier header.

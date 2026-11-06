# Changelog

All notable changes to `localplex` will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Phase 2: Library & Catalog**
  - High-performance SQLite catalog schema (`src/database/library_database.h`, `library_database.cpp`) indexing shows, seasons, episodes, and feature films.
  - Robust heuristic filename and folder parser (`src/scanner/filename_parser.h`, `filename_parser.cpp`) resolving SxxExx, 1x02, anime numbering, directory hierarchies, and release noise.
  - Background scanner (`src/scanner/library_scanner.h`, `library_scanner.cpp`) with downscaled on-disk thumbnail caching to protect memory and render budgets.
  - Virtualized catalog and shelf models (`src/models/library_model.h`, `library_model.cpp`) supporting real-time search, watched/unwatched filtering, and sorting.
  - Multi-view UI architecture in QML: Library Grid, Show Detail View (with season tabs and episode lists), and Continue Watching / Next Up shelf.
  - 5,000-item database scalability benchmark (`tools/bench/library_bench.py`).
  - Automated unit test suites for filename parsing (`test_filename_parser`) and library catalog relations (`test_library_database`).

- **Phase 1: Vertical Slice**
  - Integrated SQLite playback database (`src/database/playback_database.h`, `playback_database.cpp`) for tracking progress, completed state (at >= 90%), and per-show language preferences.
  - Automatic resume functionality with 2.5-second context rewind.
  - Remappable keybinding manager (`src/core/keybind_manager.h`, `keybind_manager.cpp`) with JSON persistence.
  - Interactive player scrubber with hover time tooltip, demuxer buffer visualization, and chapter ticks.
  - Compact popovers for audio track selection, caption/subtitle selection (including "Off"), and playback speed presets.
  - Minimalist feedback toast HUD for playback rate, volume, and track updates.
  - Mouse cursor auto-hiding synchronized with player HUD controls.
  - Multi-track test fixture generator (`tools/fixtures/generate_complex_media.sh`) producing 720p H.264 video with dual audio, subtitles, and chapter markers.
  - Playback start and seek latency benchmark (`tools/bench/playback_bench.py`).
  - Unit test suite for database and keybind operations.

- **Phase 0: Foundation & Decisions**
  - Architecture, design brief, and architectural decision records (`docs/ARCHITECTURE.md`, `docs/DESIGN.md`, `docs/DECISIONS.md`).
  - Strict privacy enforcement and pre-commit checks (`tools/git/pre-commit.sh`, `.githooks/pre-commit`).
  - Multi-platform GitHub Actions CI workflow with security scanning.
  - Bundled Inter variable font and initial SVG icon set.
  - Qt 6 + C++20 CMake build configuration embedding `libmpv` via `QQuickFramebufferObject`.
  - Native video rendering item (`MpvPlayerItem`) utilizing libmpv OpenGL render API.
  - Minimal test player UI with file loader and playback controls.

# Changelog

All notable changes to `localplex` will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-06

### Added
- **Phase 5: Packaging & Release**
  - Standalone multi-platform packaging configurations:
    - Flathub-ready Flatpak manifest (`packaging/flatpak/io.github.samgator.localplex.yml`).
    - Automated AppImage build recipe (`packaging/appimage/build-appimage.sh`) utilizing `linuxdeploy` and `linuxdeploy-plugin-qt`.
    - Windows packaging script (`packaging/windows/build-windows.ps1`) and NSIS installer definition (`packaging/windows/installer.nsi`) generating both portable `.zip` and setup executables.
    - macOS universal bundle and DMG script (`packaging/macos/package-macos.sh`, `Info.plist.in`).
  - XDG desktop entry (`assets/io.github.samgator.localplex.desktop`) and Freedesktop AppStream metainfo (`assets/io.github.samgator.localplex.metainfo.xml`).
  - Standard CMake installation targets with `GNUInstallDirs` for binary, desktop entry, metainfo, and scalable application icon.
  - Multi-platform GitHub Actions CI matrix testing Linux, macOS, and Windows with Gitleaks security verification.
  - Open source community templates: pull request template (`.github/PULL_REQUEST_TEMPLATE.md`), updated `CONTRIBUTING.md` with 10-minute setup across all three platforms, and issue templates.

- **Phase 4: Themes & Polish**
  - Token-based theme engine (`src/core/theme_manager.h`, `theme_manager.cpp`) exposing dynamic bindings for color, typography, geometry, and motion tokens with zero performance degradation.
  - 5 distinct, named bundled theme presets (`assets/themes/`): Criterion (default warm cinematic dark), Nordic Noir (arctic slate & cyan), Monochrome Archive (pure OLED black & silver), Janus Sepia (espresso & vintage brass), and Solarized Dark (precision Ethan Schoonover palette).
  - Theme specification and authoring guide in `docs/THEMES.md` with JSON schema, contrast requirements, and live-reload workflow.
  - Live-reload dev mode monitoring `~/.config/localplex/themes/` with `QFileSystemWatcher` for real-time CSS/JSON theme editing without restarting or dropping player state.
  - Accessibility & motion pass with OS reduced-motion detection and manual override collapsing all animation durations to 0 ms, high contrast focus rings, and full keyboard navigation.
  - Autoplay next episode mechanism (`LibraryDatabase::getNextEpisode`) featuring a non-intrusive 10-second countdown card with cancel and play-now actions.
  - Industrial error handling and user-first empty states per `docs/DESIGN.md` guidelines for missing files, corrupt media, unindexed folders, and zero search matches.
  - Comprehensive unit test suite for built-in and custom theme discovery and validation (`test_theme_manager`).

- **Phase 3: Import & Fix-up**
  - Event-driven filesystem folder watcher (`src/scanner/folder_watcher.h`, `folder_watcher.cpp`) leveraging `QFileSystemWatcher` with write-settle debouncing (800 ms) to trigger automated background rescanning on file changes with zero polling loops and near-zero idle CPU.
  - Native Drag-and-Drop import in QML (`src/qml/Main.qml`), supporting directory drops for automated catalog addition and background indexing, as well as single video drops for instant playback.
  - Local-first `.nfo` XML companion metadata parser (`src/scanner/nfo_parser.h`, `nfo_parser.cpp`), extracting titles, show titles, season/episode numbers, release years, and plots from companion `<basename>.nfo`, `movie.nfo`, and `tvshow.nfo` sidecars.
  - Interactive Metadata Fix-up & Correction view (`src/qml/FixupView.qml`), providing multi-select batch corrections, sequential episode numbering assignment, movie assignment, show merging, and show splitting.
  - Strict Rule 4 verification: All metadata corrections and overrides are stored exclusively in SQLite (`media_items` table); original media files and directories on disk are guaranteed to never be modified, renamed, or have their timestamps altered.
  - Automated unit test suites for NFO sidecar parsing (`test_nfo_parser`) and Rule 4-compliant metadata fix-ups (`test_fixup`).

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

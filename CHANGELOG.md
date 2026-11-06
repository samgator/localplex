# Changelog

All notable changes to `localplex` will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Phase 0: Foundation & Decisions**
  - Architecture, design brief, and architectural decision records (`docs/ARCHITECTURE.md`, `docs/DESIGN.md`, `docs/DECISIONS.md`).
  - Strict privacy enforcement and pre-commit checks (`tools/git/pre-commit.sh`, `.githooks/pre-commit`).
  - Multi-platform GitHub Actions CI workflow with security scanning.
  - Bundled Inter variable font and initial SVG icon set.
  - Qt 6 + C++20 CMake build configuration embedding `libmpv` via `QQuickFramebufferObject`.
  - Native video rendering item (`MpvPlayerItem`) utilizing libmpv OpenGL render API.
  - Minimal test player UI with file loader and playback controls.

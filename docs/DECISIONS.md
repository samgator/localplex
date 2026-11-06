# Architectural Decisions Record (ADR)

This document records the foundational architectural decisions made for localplex, with measured rationale against the project's performance budgets and quality standards.

Every external dependency must be listed in this document along with its one-line justification.

---

## Decision 001: Core Framework and UI Toolkit

- **Status:** Accepted
- **Date:** 2026-11-06

### Decision
Use **Qt 6 with Qt Quick (QML) and modern C++20**, rendering the video pipeline via **libmpv** embedded through its render API, with an embedded **SQLite** database for library indexing.

### Context and Performance Rationale
The application must satisfy stringent real-world budgets:
- Cold start under 500 ms to an interactive library.
- Idle resident memory under 150 MB for a 5,000-episode library.
- Smooth display-refresh rate scrolling on virtualized views.
- Instant, hardware-accelerated video decoding with full subtitle styling (ASS/SSA) and multi-track audio.
- Cross-platform targeting Linux first, followed by Windows and macOS, with zero compromise on Linux windowing (Wayland and X11).

### Evaluated and Rejected Alternatives

1. **Electron**
   - *Reason for rejection:* Unacceptable memory and launch latency. A bare Chromium + Node runtime consumes 180–250 MB RSS before loading user media, violating the 150 MB idle ceiling. Cold start exceeds 1.2–2.0 seconds. Video rendering via HTML5 `<video>` lacks robust ASS subtitle rendering, precise frame-stepping, and multi-track switching without heavy WebAssembly overhead.
2. **Tauri / Webview Shell (WebKitGTK / WebView2)**
   - *Reason for rejection:* WebKitGTK on Linux has historically suffered from compositing bugs, unstable hardware video acceleration on Wayland, and synchronization issues. Passing raw video frames from libmpv into a webview canvas requires high-overhead shared memory copies or WebGL texture re-uploading, increasing CPU utilization and dropping frames during scrubbing.
3. **GTK 4 / Libadwaita**
   - *Reason for rejection:* While competitive on Linux, GTK 4's packaging and rendering fidelity on Windows and macOS is fragile and visually out of place. Furthermore, embedding libmpv inside a `GtkGLArea` has edge cases with Wayland buffer synchronization and fractional scaling.

---

## Decision 002: Embedded Video Engine via libmpv Render API

- **Status:** Accepted
- **Date:** 2026-11-06

### Decision
Embed `libmpv` using the `mpv_render_context` OpenGL API inside a Qt Quick `QQuickFramebufferObject`.

### Context and Performance Rationale
- `QQuickFramebufferObject` separates the QML UI thread from the dedicated render thread, preventing UI animations from stalling during video frame rendering and vice versa.
- The `mpv_render_context_render` interface renders directly into Qt Quick's OpenGL Framebuffer Objects with zero CPU copy overhead.
- libmpv provides hardware video decoding (`hwdec=auto-safe`), full container and codec support, and native libass subtitle rendering.

---

## Decision 003: Dependency Policy and Inventory

### Policy
Dependencies are strictly minimized. Any new external library or package introduced into the codebase requires an entry in this registry with a one-line justification explaining why the functionality cannot be practically achieved with the standard C++ library or existing Qt modules.

### Current Dependencies Registry

| Dependency | License | Purpose / Justification |
| :--- | :--- | :--- |
| **Qt 6 (Core, Gui, Quick, Qml, QuickControls2, OpenGL, Sql)** | LGPLv3 / GPLv3 | High-performance virtualized UI scene graph, event loop, and cross-platform abstractions. |
| **libmpv** | LGPLv2.1+ / GPLv2+ | Industry-standard media player engine with hardware decode and universal codec support. |
| **SQLite 3** | Public Domain | Embedded, zero-configuration transactional database for local library storage. |
| **Inter Font** | SIL OFL 1.1 | Bundled high-legibility sans-serif typeface with full tabular numeric figures. |

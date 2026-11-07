# localplex Theme Specification & Authoring Guide

## Overview

localplex themes are declarative design token sets stored in plain JSON files. They define the semantic color palette, typography, structural geometry, and motion timings for the user interface.

Themes can be loaded from bundled application presets or authored by users and placed in the local user themes directory for automatic discovery and live reloading.

---

## Theme Locations

1. **Built-in Presets:**
   - Bundled in application assets (`assets/themes/*.json`).
   - Standard presets shipped with localplex:
     - `criterion` (Default): Warm cinematic dark, deep charcoal `#090a0d`, refined neutral borders `#232733`, warm amber/orange accent `#d97736`, warm soft white text `#e6e8ec`.
     - `nordic_noir`: Arctic slate, cold steel blue, and sharp glacial cyan accents.
     - `monochrome_archive`: High-contrast archival film black and silver with pure black canvas.
     - `janus_sepia`: Deep espresso, umber tones, and warm brass accents drawn from 1970s physical cinema reels.
     - `solarized_dark`: Disciplined optical balance built on Ethan Schoonover's precision color theory.

2. **User Custom Themes:**
   - Linux: `~/.config/localplex/themes/<theme_name>.json`
   - macOS: `~/Library/Application Support/localplex/themes/<theme_name>.json`
   - Windows: `%APPDATA%\localplex\themes\<theme_name>.json`

Any valid `.json` file placed in this directory is instantly discovered by localplex.

---

## Live-Reload Dev Mode

When authoring a custom theme or running localplex in development:
1. Open localplex.
2. Edit your custom theme JSON file in your editor of choice.
3. Upon saving, localplex's internal filesystem watcher detects the file modification and re-evaluates all design tokens live on the screen without restarting the application or losing playback state.

---

## Token Schema

A theme file consists of four primary token dictionaries: `colors`, `typography`, `geometry`, and `motion`.

```json
{
  "name": "theme_identifier",
  "displayName": "Human Readable Name",
  "description": "Short explanation of the aesthetic intent and references.",
  "colors": {
    "backgroundDeep": "#090a0d",
    "surfaceElevated": "#12141a",
    "surfaceCard": "#191c24",
    "surfacePopover": "#222631",
    "textPrimary": "#e6e8ec",
    "textSecondary": "#9aa0ac",
    "textDim": "#5c6370",
    "borderDivider": "#232733",
    "focusRing": "#f3f4f6",
    "accent": "#d97736",
    "accentHover": "#ea8c4f",
    "badgeMovieBg": "#1e3a5f",
    "badgeMovieFg": "#60a5fa",
    "badgeEpisodeBg": "#1e293b",
    "badgeEpisodeFg": "#93c5fd"
  },
  "typography": {
    "fontFamily": "Inter",
    "monoFontFamily": "monospace"
  },
  "geometry": {
    "radiusSmall": 4,
    "radiusMedium": 6,
    "borderWidth": 1
  },
  "motion": {
    "fastDuration": 100,
    "panelDuration": 180,
    "viewDuration": 220,
    "reducedMotion": false
  }
}
```

### Color Tokens

| Token | Description | Guidance |
| :--- | :--- | :--- |
| `backgroundDeep` | Base application canvas and video letterbox background | Pure or near pitch dark |
| `surfaceElevated`| Navigation header, titlebar chrome, and docked drawers | One step lighter than background |
| `surfaceCard`    | Media card backings, modal dialogs, and panels | Card backgrounds |
| `surfacePopover` | Floating popovers (audio/subtitle tracks, speed) | Floating HUD surface |
| `textPrimary`    | Primary headings, item titles, and active inputs | High-contrast, soft white or tinted |
| `textSecondary`  | Metadata, subtitle labels, and secondary tags | Legible mid-tone |
| `textDim`        | Placeholder text, subtle hints, and disabled buttons | Low-contrast muted tone |
| `borderDivider`  | 1px structural dividing lines and card borders | Clean definition without distraction |
| `focusRing`      | 2px keyboard focus outline | High contrast for WCAG accessibility |
| `accent`         | Progress bar, active toggle, resume badges | Semantic signature color |
| `accentHover`    | Hover state for accent buttons and scrubbers | Slightly brighter/warmer |
| `badgeMovieBg`   | Badge background for feature film tags | Distinctive category indicator |
| `badgeMovieFg`   | Badge text color for feature film tags | High contrast text against badge |
| `badgeEpisodeBg` | Badge background for episodic series tags | Distinctive category indicator |
| `badgeEpisodeFg` | Badge text color for episodic series tags | High contrast text against badge |

### Typography Tokens

- `fontFamily`: Standard sans-serif font family (defaults to bundled `Inter`).
- `monoFontFamily`: Fixed-width font for technical readouts and timecodes (defaults to `monospace` with `tnum`).

### Geometry Tokens

- `radiusSmall`: Corner radius for cards, buttons, and inputs (4px max per design brief).
- `radiusMedium`: Corner radius for floating popovers (6px max).
- `borderWidth`: Outline stroke thickness (typically 1px).

### Motion Tokens

- `fastDuration`: Hover feedback, button clicks, and subtle highlights (80-120 ms).
- `panelDuration`: Auto-hide controls and popover reveals (150-200 ms).
- `viewDuration`: Route and view switches between Library, Detail, and Player (180-240 ms).
- `reducedMotion`: Boolean flag. When set to `true` (or when detected from OS accessibility preferences), all transition durations collapse to 0 ms.

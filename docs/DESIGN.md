# localplex Design Brief

## Point of View and Mood

localplex is an archival media room, not a streaming service subscription portal.

The interface takes inspiration from mid-century film title sequences (Saul Bass, Pablo Ferro), classic physical media packaging (Criterion Collection, Janus Films, ECM Records), and the disciplined typographic hierarchy of 1970s public broadcast graphics (PBS, NHK).

It treats cinema and television as work to be watched, cataloged, and respected—not as "content" to be promoted, cross-sold, or wrapped in algorithmic noise.

The atmosphere is quiet, dark, and focused. The interface chrome is disciplined and almost entirely monochrome. The media itself—the poster artwork, photography, and video frames—provides the color.

---

## What localplex Refuses to Do

1. **No default gradients.** No diagonal purple-to-blue or magenta-to-teal gradients behind cards or navigation bars.
2. **No blanket glassmorphism.** No blurred frosted glass layered across every panel. Translucency is reserved exclusively for player HUD overlays over active video.
3. **No neon glows or blooming borders.** Focus states are crisp, high-contrast, pixel-aligned outlines.
4. **No drop shadows under posters.** Cards sit cleanly on defined background planes or subtle borders, not floating in simulated space.
5. **No emoji icons.** All iconography is drawn as custom SVG with consistent stroke geometry.
6. **No rounded dashboard cards.** Cards use tight, structural radii (4px max), never pill shapes or bubbled corners.
7. **No patronizing copy or exclamation marks.** No "Oops! Something went wrong" or "Hooray! You finished the season!" Every state is reported plainly and factually.
8. **No uninvited metadata decorations.** No unsolicited star ratings, critic badges, or trending tags obscuring cover art.

---

## Typographic System

The typographic system relies on a bundled, open-source sans-serif family (**Inter**, SIL Open Font License 1.1) paired with tabular monospace numerals for timecodes and technical readouts.

### Scale and Hierarchy

| Role | Size | Weight | Tracking (Letter Spacing) | Line Height | Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Display** | 32 px | SemiBold (600) | -0.02em (-0.64px) | 40 px | Sentence case |
| **Section Title** | 20 px | Medium (500) | -0.01em (-0.20px) | 28 px | Sentence case |
| **Item Title** | 14 px | Medium (500) | 0.00em | 20 px | Sentence case |
| **Body / Readout** | 13 px | Regular (400) | +0.005em | 18 px | Sentence case |
| **Metadata / Timestamp** | 11 px | Regular (400) | +0.02em | 16 px | Tabular numbers |
| **Label / Kicker** | 10 px | SemiBold (600) | +0.08em (+0.80px) | 14 px | All caps |

### Numerals and Timecodes
Timecodes in the player HUD and episode lists always use tabular figures (`tnum` OpenType feature) to avoid jitter during playback and scrubbing.

---

## Surface and Color Palette

The interface operates on a disciplined four-tier dark substrate:

```
[Layer 0: Deep Base]      #090a0d  (Base canvas, letterboxing)
[Layer 1: Surface Elevated] #12141a  (Sidebar, navigation chrome, header)
[Layer 2: Surface Card]    #191c24  (Card backings, dialog panels, drawer)
[Layer 3: Surface Popover] #222631  (Track selection popovers, tooltips)
```

### Contrast and Accents

- **Text Primary:** `#e6e8ec` (High legibility, soft warm white)
- **Text Secondary:** `#9aa0ac` (Metadata, subtitle labels, unselected states)
- **Text Dim:** `#5c6370` (Disabled elements, empty markers)
- **Border / Divider:** `#232733` (Subtle 1px structural separation)
- **Focus Ring:** `#f3f4f6` (2px solid border with 2px offset, uncompromising accessibility)
- **Accent (Playback & Progress):** `#d97736` (Warm cinematic amber; used sparingly for scrubber position, active track marker, and resume indicators)

---

## Spatial System and Grid

All layout dimensions align to a strict 4px grid.

- **Spacing scale:** `4px`, `8px`, `12px`, `16px`, `24px`, `32px`, `48px`, `64px`.
- **Card proportions:**
  - Movie / Show Posters: Standard theatrical **2:3 ratio** (e.g. 160×240px, 200×300px).
  - Episode Thumbnails: Standard **16:9 ratio** (e.g. 240×135px, 320×180px).
- **Corner Radii:**
  - Base cards: `4px`.
  - Popovers and menus: `6px`.
  - Buttons and input fields: `4px`.

---

## Iconography

Icons are custom SVGs designed on a 20×20 pixel viewport with a 2px padding ring (effective 16×16 drawing area).

- **Stroke:** Uniform `1.5px` stroke weight.
- **Join / Cap:** `round` stroke linejoin and linecap for clean rendering at standard DPI.
- **Style:** Outlined by default; solid fills used only for active state toggle (e.g., Playing vs Paused).
- **Format:** Checked in as standalone SVG assets under `assets/icons/`.

---

## Motion and Transitions

Motion is sparse, intentional, and quick. It serves only to orient the user between views or signal state changes.

- **Fast feedback (hover, button press):** `100ms`, `cubic-bezier(0.2, 0.0, 0.0, 1.0)`.
- **Panel reveal / HUD auto-hide:** `180ms`, `cubic-bezier(0.0, 0.0, 0.2, 1.0)`.
- **View transitions (Library to Detail):** `220ms`, `cubic-bezier(0.16, 1.0, 0.3, 1.0)`.
- **Bounces:** Never. No spring oscillations or bouncy easing.
- **Accessibility:** If the OS `prefers-reduced-motion` setting is detected, transition durations collapse to `0ms`.

---

## States and Copywriting

Copy is concise, unambiguous, and human.

### Empty States
- **Library empty:**
  - *Heading:* No media folders connected.
  - *Body:* Add a folder containing your movies or television shows to begin.
  - *Action:* Add folder
- **Search without results:**
  - *Heading:* No matches found.
  - *Body:* No files in your library match "{query}". Check the spelling or search by show title.

### Error States
- **Corrupt or unreadable file:**
  - *Message:* The file could not be decoded. The container or video stream may be corrupt.
- **Missing file:**
  - *Message:* The file at "{path}" could not be located. It may have been moved or the drive disconnected.

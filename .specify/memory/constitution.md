# Perception Constitution

## Core Principles

### I. Spec-First (NON-NEGOTIABLE)
Every feature starts with `specify` → `plan` → `tasks` from the spec-kit workflow. No implementation before a ratified spec exists in `specs/<feature>/`.

### II. Local Design Source
The UI's only design source of truth is `docs/design/mockups/` (pure-local scheme; NO figma extension, NO Figma REST/MCP, no `FIGMA_PAT`).
- Before `specify`: read `docs/design/mockups/**/preview.png` + `notes.md`, and write visual requirements into `specs/<feature>/spec.md`.
- At `plan`: order UI work against the mockups.
- At `tasks`: each UI task references the corresponding `NNN-<界面名>/` as its completion standard.
- After implementation: self-check by comparing screenshots to mockups; fix discrepancies.

### III. Layered Core (NON-NEGOTIABLE)
Data flows through four decoupled layers in `src/core`, each independently testable:
1. `model/` — format-agnostic data model (curve data + structure data).
2. `io/` — format readers behind a registry (extensible; a new format = a new reader, no core changes).
3. `process/` — data transforms (resample, unit conversion, projections).
4. `event/` — event bus (publish-subscribe).

**Event-driven updates**: render and UI never poll or call data directly on change; they subscribe to `event/` and redraw on events (e.g. `DataSetChanged`, `StructureChanged`, `SelectionChanged`).

### IV. Python Command-Driven Data Access (NON-NEGOTIABLE)
Every data CRUD operation (load, add/remove curves, transform, query, export) MUST go through the Python command layer (`perception_py`, exposed via pybind11). UI actions that change data are translated into Python commands; the UI never mutates core data bypassing the command layer. Pure-UI changes (layout, theme, panel visibility, selection highlight) are exempt.
Rationale: a single scriptable API for both GUI and headless automation; tests reuse the same commands.

### V. Test-First, Dual Layout
- C++ unit tests (`tests/cpp/`) are mandatory for `src/core` via CTest: tests written → approved → fail → then implement. Red-Green-Refactor strictly enforced.
- Python tests (`tests/python/`, pytest) exercise the command layer through `perception_py`, mirroring real usage (load → transform → query → export).
- `ctest` AND `pytest` green required before merge.

## Design Constraints

- Deep dark theme with a fixed color palette defined in `docs/design/design.md` (main window / Dock / curve view / structure view).
- Qt5 Widgets + Dock layout + QSS; no external UI frameworks.
- All text I/O (file parsing, logs) is UTF-8.
- Front-end / back-end separation: UI holds only display abstractions (e.g. `ICurveChart`), never raw VTK/data internals; VTK lives in `render/` only.

## Safety Constraints

- All file inputs validated before use: path existence, extension allowlist, size limit, parse errors reported as typed errors — never crash the app or the Python interpreter.
- No data mutation without an accompanying event on the bus; consumers must tolerate partial/streamed updates.
- Python command layer is the only trust boundary for external input; C++ core treats all command args as untrusted.

## Extensibility

- New curve format (.plt/.csv/.dat/...) → add an `ICurveReader` in `io/readers/` and register it; no changes to `model/` or consumers.
- New structure format (.tdr/...) → add an `IStructureReader` likewise.
- New transform → add a transform to `process/`; pipelines compose.

## Quality Gates

- `ctest` and `pytest` green required before merge.
- UI screenshots compared against mockups at M2/M4 milestones.
- One feature per branch; merges to `main` via PR.

## Governance

Constitution supersedes other practices. Amendments require documentation + approval.
Mockup conventions live in `docs/design/mockups/README.md` (this file links there).

**Version**: 1.1 | **Ratified**: 2026-08-23 | **Last Amended**: 2026-08-23

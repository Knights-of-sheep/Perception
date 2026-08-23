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

### III. Library-First Architecture
`src/core` is a standalone, self-contained, testable library with no UI dependencies. `src/ui` consumes `src/core` via a stable API. `src/python` exposes the same core via pybind11.

### IV. Test-First
CTest mandatory for `src/core`: tests written → approved → fail → then implement. Red-Green-Refactor strictly enforced.

## Design Constraints

- Deep dark theme with a fixed color palette defined in `docs/design/design.md` (main window / Dock / curve view).
- Qt5 Widgets + Dock layout + QSS; no external UI frameworks.
- All text I/O (file parsing, logs) is UTF-8.

## Quality Gates

- `ctest` green required before merge.
- UI screenshots compared against mockups at M2/M4 milestones.
- One feature per branch; merges to `main` via PR.

## Governance

Constitution supersedes other practices. Amendments require documentation + approval.
Mockup conventions live in `docs/design/mockups/README.md` (this file links there).

**Version**: 1.0 | **Ratified**: 2026-08-23 | **Last Amended**: 2026-08-23

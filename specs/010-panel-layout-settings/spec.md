# Feature Specification: Panel Layout Settings

**Feature Branch**: `010-panel-layout-settings`

**Created**: 2026-08-31

**Status**: Draft

**Input**: User description: "菜单栏上view增加功能 panel settings功能，用来控制data property pyshell三个面板的布局；左右两侧只能放 data或者 property面板（左右每一侧最多只能放一个面板）；底部只能放pyshell面板；这里布局设置支持四种模式，且支持设置三个面板的显隐，三个面板具有expand特性，如果某个面板隐藏，相关面板视情况来做出expand；支持预览；"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Open Panel Settings and Choose a Layout Mode (Priority: P1)

As a user, I want to open the Panel Settings from the View menu and select one of four predefined panel layout modes, so that I can quickly arrange the Data, Property, and PyShell panels into a comfortable workspace.

**Why this priority**: This is the core interaction of the feature. Without it, users cannot access the layout controls at all.

**Independent Test**: This story can be tested independently by opening the Panel Settings dialog, selecting each of the four modes, and verifying that the Data, Property, and PyShell panels move to the expected screen regions.

**Acceptance Scenarios**:

1. **Given** the application is open with the default layout, **When** the user opens the View menu and selects "Panel Settings", **Then** a settings dialog appears showing the four layout modes and visibility toggles for Data, Property, and PyShell.
2. **Given** the Panel Settings dialog is open, **When** the user selects a mode that places a panel on the left and another on the right, **Then** the Data and Property panels are arranged on the left and right sides (one per side), and the PyShell panel follows the mode's size form — full-width at the bottom for `*WithConsole` modes, or an embedded narrow bar below the plot (side panels keep full height) for `*Only` modes.
3. **Given** the user has selected a layout mode, **When** the user confirms the settings, **Then** the main window reflects the selected layout and the dialog closes.

---

### User Story 2 - Toggle Panel Visibility with Auto-Expand (Priority: P2)

As a user, I want to show or hide individual panels independently, so that I can focus on the content I need while the remaining panels automatically expand to use the freed space.

**Why this priority**: Visibility control gives users flexibility beyond the four presets and keeps the workspace uncluttered without leaving empty gaps.

**Independent Test**: This story can be tested independently by hiding one or more panels and verifying that the visible panels expand into the available space, then showing them again and verifying that space is reclaimed.

**Acceptance Scenarios**:

1. **Given** a layout with a panel on the left and the main workspace in the center, **When** the user hides the left panel, **Then** the main workspace expands horizontally to fill the space previously occupied by the left panel.
2. **Given** a layout with the PyShell panel at the bottom, **When** the user hides the PyShell panel, **Then** the main workspace and any side panels expand vertically to fill the space previously occupied by the bottom panel.
3. **Given** all three panels are visible, **When** the user hides two panels, **Then** the remaining visible panel and the main workspace expand to occupy the entire available area.

---

### User Story 3 - Live Preview Before Applying (Priority: P2)

As a user, I want to see the layout changes as I adjust settings in the Panel Settings dialog, so that I can decide whether to keep or cancel the changes before they are finalized.

**Why this priority**: Preview reduces trial-and-error and gives users confidence that the chosen layout will match their expectations.

**Independent Test**: This story can be tested independently by changing layout modes or visibility toggles inside the settings dialog and observing that the main window updates immediately.

**Acceptance Scenarios**:

1. **Given** the Panel Settings dialog is open, **When** the user switches from one layout mode to another, **Then** the main window layout updates immediately to reflect the new mode.
2. **Given** the Panel Settings dialog is open, **When** the user toggles a panel's visibility off, **Then** the panel disappears and the remaining panels expand in real time.
3. **Given** the user has made layout changes in the dialog, **When** the user clicks "Cancel", **Then** the main window returns to the layout that existed before the dialog was opened.

---

### Edge Cases

- What happens when the user tries to hide all three panels? The system should allow it and give the main workspace the full available area.
- What happens when a layout mode expects a panel on the left/right but that panel is hidden? The main workspace should expand to fill the empty side.
- What happens when the user switches modes while one or more panels are hidden? The new mode should still apply, and visible panels should follow the new positions while hidden panels remain hidden.
- What happens when the application is resized while panels are hidden? The expanded panels should continue to fill the available space proportionally.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The View menu MUST provide a "Panel Settings" entry that opens the panel layout settings dialog.
- **FR-002**: The Panel Settings dialog MUST offer four predefined layout modes covering the valid arrangements of the Data, Property, and PyShell panels. The modes differ in the left/right assignment (Data ↔ Property) and in the PyShell size form: `*Only` modes place PyShell as a non-full-width bar embedded below the plot (side panels keep full height), while `*WithConsole` modes place PyShell full-width at the bottom (side panels sit above it). All four modes include PyShell by default.
- **FR-003**: In every layout mode, each side region (left and right) MUST contain at most one panel, and that panel MUST be either the Data panel or the Property panel. The bottom region MUST contain only the PyShell panel when it is present.
- **FR-004**: The Panel Settings dialog MUST allow the user to independently toggle the visibility of the Data, Property, and PyShell panels.
- **FR-005**: When a panel is hidden, the remaining visible panels and the main workspace MUST expand to fill the newly available space without leaving empty gaps or causing overlap.
- **FR-006**: Changes made inside the Panel Settings dialog MUST be previewed in the main window in real time before the user confirms them.
- **FR-007**: Layout mode and visibility settings MUST persist across application sessions so that the workspace is restored on restart.
- **FR-008**: If a user action would produce an invalid layout (for example, placing a panel in an unsupported region), the system MUST either prevent the action or automatically correct it to the nearest valid state.

### Key Entities *(include if feature involves data)*

- **Panel Layout Setting**: The current workspace arrangement, consisting of a selected layout mode and the visibility state of each panel. It is persisted and restored across sessions.
- **Layout Mode**: A predefined arrangement rule that specifies where the Data and Property panels appear on the left/right sides and the PyShell size form (full-width at the bottom vs. embedded below the plot). Four modes are supported.
- **Panel**: One of the three workspace panels: Data panel, Property panel, or PyShell panel. Each panel has a visibility state and participates in expand behavior when adjacent panels are hidden.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user can open Panel Settings and switch to any of the four layout modes in three interactions or fewer.
- **SC-002**: After a panel is hidden or shown, the workspace layout completes its expand/contract adjustment within 200 milliseconds.
- **SC-003**: Across all four layout modes and all combinations of panel visibility, the main workspace and visible panels occupy the entire available area without overlap or uncovered gaps.
- **SC-004**: Changes made in the Panel Settings dialog are reflected in the main window preview within 100 milliseconds.
- **SC-005**: On application restart, the last selected layout mode and visibility states are restored correctly.

## Assumptions

- The three target panels (Data, Property, PyShell) already exist in the application; this feature only controls their arrangement and visibility.
- "Preview" means real-time updates in the main window while the dialog is open, with changes reverted if the user cancels.
- Custom drag-and-drop layout editing is out of scope for this feature; only the four predefined modes plus visibility toggles are supported.
- Layout persistence applies to the current user profile or local application settings; multi-user synchronization is out of scope.
- Hiding a panel does not destroy its content or state; it only removes the panel from view and allows other panels to expand.

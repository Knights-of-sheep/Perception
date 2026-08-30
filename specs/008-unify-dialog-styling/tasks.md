# Tasks: 弹窗样式层次化统一 + 布局设置弹窗优化

**Input**: Design documents from `/specs/008-unify-dialog-styling/`

**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, contracts/

**Tests**: 按宪法「测试先行」要求纳入测试任务（WS1 单测已实现待验证；WS2 布局单测先红后绿）。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- 本 feature 为 Qt desktop-app，路径：`src/ui/`（界面层）、`tests/cpp/`（CTest）、`docs/design/`（设计文档）、`specs/004-dock-layout-manager/`（跨 feature 同步）

---

## 状态说明（2026-08-30）

- **WS1（US1/US2/US3 弹窗样式层次化）代码已实现、未提交**：`src/ui/theme/theme_dialog_layer.{h,cpp}`、`src/ui/themed_message_box.{h,cpp}`、`src/ui/theme/file_dialog_qss.{h,cpp}`、`theme_types.h`（dialogBg 字段）、`theme_manager.cpp`（@dialogBg@ token）、`theme_template.qss`（QDialog/QMessageBox 拆分）、`MainWindow.cpp`/`log_settings_controller.cpp`（5 处替换）、`tests/cpp/theme_dialog_layer_test.cpp` 均已落地并注册进 CMake。WS1 阶段任务为**核对契约、验证红-绿、补手动验收**。
- **WS2（US4 布局设置）全部待实现**：`layout_manager` 语义修订、弹窗重构、预览组件、容器计数、004 文档同步。

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 建立基线，确认当前分支状态可编译

- [X] T001 Run `scripts/build.ps1` to confirm the current branch (including uncommitted WS1 implementation: `src/ui/theme/theme_dialog_layer.cpp`, `src/ui/themed_message_box.cpp`, `src/ui/theme/file_dialog_qss.cpp`, `src/ui/theme/theme_manager.cpp`, `src/ui/theme/theme_template.qss`) compiles cleanly; record the baseline before any red-green cycles

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: WS1 弹窗层基础实现（阻塞 US1/US2/US3 的视觉验收；US4 不依赖但共享基线）

- [X] T002 [P] Verify WS1 core implementation against `specs/008-unify-dialog-styling/contracts/dialog-style-contract.md` §1: `deriveDialogBg` rules in `src/ui/theme/theme_dialog_layer.cpp` (Dark +8 HSL clamp 70 / Light −8 clamp 30 / HC invalid color / contrast protection ≥4.5:1 with ≥2pt floor), `@dialogBg@` token + fallback chain in `src/ui/theme/theme_manager.cpp` (explicit → derived → windowBg), QDialog/QMessageBox rule split + 1px border in `src/ui/theme/theme_template.qss`, `dialogBg` field in `src/ui/theme/theme_types.h`; fix any mismatch
- [X] T003 Run `ctest --test-dir build -R theme_dialog_layer --output-on-failure` to verify all 25 themes satisfy derived constraints (SC-001 automatic part: |ΔL| ≥ 8 for Dark/Light, contrast ≥ 4.5:1 vs text, HC border ≥ 7:1); if red, fix implementation in `src/ui/theme/theme_dialog_layer.cpp` until green

---

## Phase 3: User Story 1 (P1) — 弹窗悬浮可辨

**Goal**: 弹窗背景与主界面背景存在可感知层次，边界清晰可辨

**Independent Test**: 依次打开全部弹窗类型，弹窗背景与主界面背景存在可感知差异，所有弹窗表现一致（quickstart §3.1）

- [ ] T004 [P] [US1] Manual matrix (quickstart §3.1): open all 10 dialog types (Help/About/Open/Export image/Export command/Log path/Layout settings/Export fail warning/Clear confirm/Clear fail warning) on Dark Classic, Light Classic, HC Black; verify background layer vs main window, 1px border, readability, drag, close
- [ ] T005 [US1] Verify Edge Cases: light theme layer direction is darker than main window (not inverted), extreme small/large dialog sizes keep layer+border+title bar intact, stacked dialogs keep relative hierarchy without swallowing each other

---

## Phase 4: User Story 2 (P2) — 弹窗随主题协调

**Goal**: 弹窗随主题切换自动适配，配色协调无孤立色块

**Independent Test**: 依次切换全部内置主题，逐一打开各弹窗类型，配色协调无刺眼对比（quickstart §3.2）

- [ ] T006 [P] [US2] Hot-switch verification (quickstart §3.2): keep a dialog open, switch themes across families (Dark → Light → HC → Dark); verify background/title bar/border follow instantly with no stale colors or conflicts
- [ ] T007 [US2] Verify contrast-protection case and low-contrast palettes (e.g., solarized-dark): sample 25 themes via `tests/cpp/theme_dialog_layer_test.cpp` coverage; visually confirm dialogBg/text/controls remain readable with no isolated or jarring color blocks

---

## Phase 5: User Story 3 (P3) — 标题栏统一

**Goal**: 所有弹窗标题栏外观一致，无系统原生标题栏残留

**Independent Test**: 并排打开 ≥3 类弹窗，标题栏高度/图标/字号/按钮位置一致（quickstart §3.1）

- [X] T008 [P] [US3] Verify `ThemedMessageBox` implementation against contract §3 (`src/ui/themed_message_box.{h,cpp}`): frameless (Qt::FramelessWindowHint), reuses `buildDialogTitleBar`, Esc/close returns defaultButton, button text via standard mapping, drag support
- [X] T009 [US3] Search codebase for residual static `QMessageBox::warning`/`QMessageBox::question` calls (must be only the 5 replaced call sites in `src/ui/MainWindow.cpp` ×2 and `src/ui/log/log_settings_controller.cpp` ×3 now using `showThemedMessageBox`); eliminate any leftover
- [ ] T010 [US3] Manual side-by-side comparison: open ≥5 dialog types, verify title bar height/icon/title font size/close button position/bottom divider identical (SC-003)

---

## Phase 6: User Story 4 (P1) — 布局设置弹窗优化

**Goal**: 按行排=N×1、按列排=1×N、网格=比例+优先级轴约束；弹窗分段按钮组 + 实时预览 + 恢复默认 + 间隙宽度；004 语义同步（2026-08-30 再次澄清修订：By Row=一列多行 N×1、By Column=一行多列 1×N）

**Independent Test**: ≥3 个子窗口切换三种模式并设置不同优先级/约束，截图比对排布与控件显隐；渲染内容与视图状态保留（quickstart §3.4）

**红线（TDD）**：

- [X] T011 [P] [US4] RED: revise `tests/cpp/layout_manager_test.cpp` to new semantics — Row mode always N×1 ignoring constraints (n=2→2×1, n=5→5×1), Column mode always 1×N (n=2→1×2, n=5→1×5), Grid unconstrained proportional (unchanged: 3→2×2, 5→3×2), Grid+Row priority honors only maxRows (maxRows=2, n=5→2×3), Grid+Column priority honors only maxCols (maxCols=2, n=5→3×2), stale inactive-axis constraints ignored, `constraintAxis` full matrix, spacing cellSize deduction; run `ctest --test-dir build -R layout_manager` and confirm RED
- [X] T014 [US4] GREEN: run `ctest --test-dir build -R layout_manager --output-on-failure` until all revised cases pass (after T012/T013)

**实现（顺序依赖 T011 红线）**：

- [X] T012 [US4] Implement `ConstraintAxis {None, Row, Column}` enum + `constraintAxis(const LayoutConfig&) const` declaration in `src/ui/subwindow/layout_manager.h`; update `LayoutConfig` doc comments to constraint-axis semantics
- [X] T013 [US4] Implement in `src/ui/subwindow/layout_manager.cpp`: revise `computeGrid` — Row→{n,1}, Column→{1,n} (ignore maxRows/maxCols/gridDirection), Grid uses constraint axis only (`gridDirection==Row`: maxRows>0→rows=min(n,maxRows),cols=ceil(n/rows) else proportional; `gridDirection==Column`: mirrored with maxCols); keep proportional formulas and `windowCount<=0` empty grid unchanged; implement `constraintAxis()`
- [X] T015 [P] [US4] Add `int visibleSubwindowCount() const` to `src/ui/subwindow/subwindow_container.{h,cpp}` (count of views not hidden and not bypassed by maximization, mirroring `relayout` visible-list logic)
- [X] T016 [P] [US4] Create `LayoutPreviewWidget` (`src/ui/subwindow/layout_preview_widget.{h,cpp}`): QWidget subclass with `setPreviewCount(int)` + `setConfig(const LayoutConfig&)`, paintEvent draws cells via `LayoutManager::computeGrid` + `cellRect` (reuse, no new geometry logic) with @panelBg@ cells + 1px @border@ on @viewBg@ background, empty-state text for n=0, full-cell for n=1
- [X] T017 [US4] Refactor `LayoutSettingsDialog` (`src/ui/subwindow/layout_settings_dialog.{h,cpp}`): replace QComboBox with segmented button group (QButtonGroup + 3 checkable QPushButton: Grid/By Row/By Column, click-to-apply), control visibility driven by `constraintAxis` (Row/Column hide priority+maxRows+maxCols; Grid shows priority radios + only the axis-matching spinbox), add gap-width QSpinBox (range 0–50, default 6, binds `spacing`), add Restore Default button (resets to Grid + Row priority + no constraints + sameSize off + gap 6 and emits configChanged), embed `LayoutPreviewWidget`, keep hidden spinbox values in `current()` (ignored by computeGrid)
- [X] T018 [US4] Update `src/ui/theme/theme_template.qss` layout-settings rules: replace `QDialog#layoutSettingsDialog QComboBox` rule with segmented QPushButton styles (checked state via @accent@), add `layoutPreviewWidget`/preview-label QSS using @viewBg@/@panelBg@/@border@ tokens
- [X] T019 [US4] Wire up `MainWindow::openLayoutSettings` in `src/ui/MainWindow.cpp`: call `layoutSettingsDialog_->setPreviewCount(subwindowContainer_->visibleSubwindowCount())` before show, and connect `SubwindowContainer::subwindowCountChanged` to keep preview count in sync while dialog is open
- [X] T020 [P] [US4] Sync-revise `specs/004-dock-layout-manager/spec.md` and `data-model.md`: US2/FR-004~006 semantics (Row=N×1, Column=1×N, Grid=proportional+priority), FR-011 constraint visibility, FR-015 spacing configurable (default 6, range 0–50)
- [X] T021 [US4] Update `docs/design/ui-guidelines.md` §4: layout-settings dialog control spec (segmented button group, constraint visibility matrix, live preview, gap width, restore default)

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: 手动验收矩阵 + 全量回归 + 文档同步核对

- [ ] T022 [P] Execute quickstart §3.4 layout-settings manual matrix: three-mode arrangement semantics (N×1 / 1×N / proportional grid at 3 and 5 subwindows), control visibility matrix per mode, live preview matches real arrangement, restore default returns to defaults, gap width 0/6/50 behaves in all modes, empty-state (n=0) and single-cell (n=1) preview
- [X] T023 [P] Full regression (constitution gate): `scripts/build.ps1` + `ctest --test-dir build` + `pytest tests\python` all green
- [X] T024 [P] Documentation consistency check: `docs/design/ui-guidelines.md` and root `README.md` synced with final feature state; `specs/004-dock-layout-manager/` docs consistent with implementation (constitution workflow rule)

---

## Dependency Graph

```
Phase 1 (Setup) T001
   └── Phase 2 (Foundational) T002 → T003
          ├── Phase 3 (US1) T004 → T005
          ├── Phase 4 (US2) T006 → T007
          ├── Phase 5 (US3) T008 → T009 → T010
          └── Phase 6 (US4) T011 → T012 → T013 → T014   (T011 红线 → T012/T013 实现 → T014 绿)
                             ├── T015 (container count, [P] 独立)
                             ├── T016 (preview widget, 依赖已实现的 LayoutManager)
                             ├── T017 (dialog 重构, 依赖 T013/T016)
                             ├── T018 (QSS, 依赖 T017)
                             ├── T019 (MainWindow 接线, 依赖 T015/T017)
                             ├── T020 (004 文档, [P] 独立)
                             └── T021 (ui-guidelines, [P] 独立)
   └── Phase 7 (Polish) T022/T023/T024
```

### Within Each User Story

- WS1 阶段（US1/2/3）：先核对契约（T002/T008），再跑自动化验证（T003），最后手动矩阵（T004-T010）
- US4：**先红**（T011）→ 实现（T012/T013）→ **后绿**（T014）；随后弹窗重构（T017）与接线（T019）依赖计算层
- Story complete before moving to next priority

### Parallel Opportunities

- Foundational：T002/T003 并行（T003 依赖构建基线）
- US1/US2/US3 的验证任务相互独立（不同弹窗行为面），可并行
- US4：T011（测试红线）与 T015/T016/T020/T021 并行（不同文件）；T015 与 T016 并行
- Polish：T022/T023/T024 并行（手动/回归/文档互不冲突）

---

## Parallel Example: User Story 4

```bash
# 红线测试与独立实现并行（不同文件）：
Task: "T011 RED: revise tests/cpp/layout_manager_test.cpp to new semantics"
Task: "T015 Add visibleSubwindowCount() to src/ui/subwindow/subwindow_container.{h,cpp}"
Task: "T016 Create LayoutPreviewWidget in src/ui/subwindow/layout_preview_widget.{h,cpp}"
Task: "T020 Sync-revise specs/004-dock-layout-manager/spec.md and data-model.md"

# 计算层实现（依赖 T011 红线确认后）：
Task: "T012 Add ConstraintAxis + constraintAxis declaration in src/ui/subwindow/layout_manager.h"
Task: "T013 Revise computeGrid semantics in src/ui/subwindow/layout_manager.cpp"

# 弹窗与接线（依赖 T013/T016）：
Task: "T017 Refactor LayoutSettingsDialog in src/ui/subwindow/layout_settings_dialog.{h,cpp}"
Task: "T018 Update layout-settings QSS rules in src/ui/theme/theme_template.qss"
Task: "T019 Wire preview count in MainWindow::openLayoutSettings in src/ui/MainWindow.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 + User Story 4)

1. Complete Phase 1: Setup（构建基线）
2. Complete Phase 2: Foundational（WS1 契约核对 + theme_dialog_layer 单测绿）
3. Complete Phase 3: User Story 1（弹窗悬浮可辨 = 核心痛点，WS1 已实现，验证即达主价值）
4. **STOP and VALIDATE**: User Story 1 独立验收
5. Complete Phase 6: User Story 4（P1 新增核心：布局语义 + 弹窗界面）→ 独立验收

### Incremental Delivery

1. Complete Setup + Foundational → 弹窗层次基线就绪
2. Add User Story 1 → 验证通过 → 弹窗可辨（主价值）
3. Add User Story 2 → 主题协调 → 独立验收
4. Add User Story 3 → 标题栏统一 → 独立验收
5. Add User Story 4 → 布局设置重排+界面升级 → 独立验收
6. Polish：手动矩阵 + 全量回归 + 文档核对

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1 + 2 + 3（WS1 验证线）
   - Developer B: User Story 4（计算层红线 → 实现 → 弹窗重构 → 接线）
3. Stories complete and integrate independently（US4 文件面独立：layout_manager/dialog/container/preview；MainWindow 改动区域与 US3 已替换区域不重叠）

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing (T011 RED → T012/T013 → T014 GREEN)
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence

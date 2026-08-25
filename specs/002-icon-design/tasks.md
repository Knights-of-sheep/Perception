# Tasks: 功能按钮图标设计

**Input**: Design documents from `/specs/002-icon-design/`

**Prerequisites**: plan.md, spec.md（3 个用户故事）、research.md、data-model.md、contracts/（icon-style-spec / icon-function-map / conformance-checklist）、quickstart.md

**Tests**: 本功能为设计 + 资源交付，无 core 测试套件变更。验收采用规范符合性评审（conformance-checklist 31 项）+ 语义识别盲测 + 资源编译验证 + 既有 ctest 回归（见 quickstart.md 场景 1–5）。不生成 TDD 测试任务。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- 图标源资源：`src/ui/theme/icons/`（`actions/` 功能图标、`app/` 应用图标、`templates/` 绘制模板）
- 设计规范：`docs/design/icon-spec.md`（正式规范文档）；可检查契约：`specs/002-icon-design/contracts/`
- 视觉稿：`docs/design/mockups/005-icon-set/`
- 校验脚本：`scripts/check_icons.py`

---

## Phase 1: Setup（共享基础设施）

**Purpose**: 目录骨架、构建环境风险验证、映射表骨架

- [X] T001 Create directory skeleton for icon resources: `src/ui/theme/icons/actions/`, `src/ui/theme/icons/app/`, `src/ui/theme/icons/templates/`, and mockups dir `docs/design/mockups/005-icon-set/`
- [X] T002 [P] Verify Qt5Svg availability in build environment (check for Qt5Svg.dll in Qt install / CMake `find_package(Qt5 Svg)`); record result in `specs/002-icon-design/research.md` §6 risk register to finalize SVG vs PNG rendering strategy
- [X] T003 [P] Create `icon-map.yaml` skeleton at `src/ui/theme/icons/icon-map.yaml` per `specs/002-icon-design/contracts/icon-function-map.md` schema (version + empty entries list + category enum `file/edit/view/analysis/animation/tools`)

---

## Phase 2: Foundational（阻塞前置）

**Purpose**: 规范定稿 + 绘制模板 + 校验脚本。**⚠️ 未完成前不得开始任何用户故事。**

- [X] T004 Author the official icon design spec document `docs/design/icon-spec.md` (style/grid/size slots 16-24-32/palette tokens from `docs/design/ui-guidelines.md` §3.1/five states/naming/app icon), keeping it consistent with `specs/002-icon-design/contracts/icon-style-spec.md`
- [X] T005 [P] Create three-size SVG drawing templates (16/24/32px with 12×12 safe zone, stroke widths 2/2.5/3px, round cap/join) at `src/ui/theme/icons/templates/template-{16,24,32}.svg`
- [X] T006 [P] Write icon conformance checker `scripts/check_icons.py` (palette whitelist scan from `ui-guidelines.md` tokens, naming rules per icon-function-map.md, coverage check icon↔map entries) — must exit non-zero on violations

---

## Phase 3: User Story 1 — 统一的图标设计规范 (P1)

**Goal**: 交付可逐项核验的图标设计规范，成为全部图标的唯一设计依据。

**Independent Test**: 评审人员对照 `docs/design/icon-spec.md` 任取一枚图标逐项核验（风格/网格/尺寸/色板/状态），得出明确通过与不通过结论；规范中所有取值（像素网格、色值、描边宽度）为具体数值而非模糊描述。

- [X] T007 [US1] Populate `icon-map.yaml` at `src/ui/theme/icons/icon-map.yaml` with full mapping entries from spec「对标功能清单」six categories (file/edit/view/analysis/animation/tools), each with `icon_id`/`semantic`/`category`/`benchmark_ref`/`states`/`sizes`
- [X] T008 [P] [US1] Align `specs/002-icon-design/contracts/icon-style-spec.md` with `docs/design/icon-spec.md` to establish single source of truth (both documents must not contradict)
- [X] T009 [US1] Self-review the icon spec: run `scripts/check_icons.py` naming/category validation against `src/ui/theme/icons/icon-map.yaml`; ensure every required value (16px slot, palette tokens, stroke widths) is concrete

**Story completion**: 规范文档 + 映射表 + 校验脚本通过命名/枚举校验；契约与规范文档无矛盾。

---

## Phase 4: User Story 2 — 核心功能按钮图标全覆盖 + 应用主图标 (P2)

**Goal**: 全部六大类别功能按钮均有语义对应、风格统一的图标；交付应用主图标。

**Independent Test**: 语义识别盲测（≥10 枚 × 3 人，正确率 ≥90%，SC-003）；映射表与图标文件一一对应无空缺；应用图标 16px 档可辨（SC-006）。

- [X] T010 [P] [US2] Draw "文件与数据" category icons (open/save/export-screenshot/export-data/save-session/close/remove etc., ~7) as SVG at `src/ui/theme/icons/actions/file-*.svg` per templates & palette
- [X] T011 [P] [US2] Draw "编辑" category icons (undo/redo/delete-selection, 3) as SVG at `src/ui/theme/icons/actions/edit-*.svg`
- [X] T012 [P] [US2] Draw "视图与相机" category icons (rotate/pan/zoom-in/zoom-out/zoom-box/fit-screen/reset-camera/view-x/view-y/view-z/display-2d/display-3d/layer-visibility/multi-view, ~14) as SVG at `src/ui/theme/icons/actions/view-*.svg`
- [X] T013 [P] [US2] Draw "数据操作与分析" category icons (select-point/select-cell/select-region/cutline/probe/annotate/extract/clip/slice/contour/threshold/warp/curve-add/curve-remove/axis-settings/legend, ~16) as SVG at `src/ui/theme/icons/actions/analysis-*.svg`
- [X] T014 [P] [US2] Draw "动画与播放" category icons (play/pause/first-frame/last-frame/step-forward/step-backward/param-scan, ~7) as SVG at `src/ui/theme/icons/actions/animation-*.svg`
- [X] T015 [P] [US2] Draw "工具与设置" category icons (settings/measure/help/about, ~5) as SVG at `src/ui/theme/icons/actions/tools-*.svg`
- [X] T016 [P] [US2] Design app icon (brand mark, ACCENT + FG_TEXT palette, line-outline language) as SVG source at `src/ui/theme/icons/app/app-icon.svg` per icon-style-spec A-01~A-02
- [X] T017 [US2] Render PNG bitmaps (16/24/32px for all actions; app icon 16/24/32/48/64/128/256 + Windows `.ico`) from SVG into `src/ui/theme/icons/actions/` and `src/ui/theme/icons/app/` per research.md §6 decision (fall back to full-PNG if T002 found no Qt5Svg)
- [X] T018 [US2] Run `scripts/check_icons.py` — must pass palette/naming/coverage checks; update `src/ui/theme/icons/icon-map.yaml` until coverage rule 1&2 (icon↔map one-to-one, required_icon coverage) fully green
- [X] T019 [US2] Run semantic recognition blind test per conformance-checklist 评审记录（≥10 枚图标 × 3 人，正确率 ≥90%，SC-003）+ app icon 16px legibility check (SC-006); record results in `specs/002-icon-design/checklists/requirements.md` and conformance-checklist 结论区

**Story completion**: 全部图标 + 映射表校验全绿；盲测 ≥90%；应用图标多尺寸交付齐备。

---

## Phase 5: User Story 3 — 深色主题下的状态表现 + Mockups 与资源集成 (P3)

**Goal**: 五态在深色主题清晰可辨；mockups 直观展示；图标资源可编译并接入窗口图标。

**Independent Test**: mockups 中五态视觉差异清晰（SC-005）；`.qrc` 编译链接通过；运行 `Perception.exe` 任务栏/标题栏显示应用图标。

- [X] T020 [P] [US3] Create icon set visual mockup `docs/design/mockups/005-icon-set/preview.png` showing all icons × five states on dark theme background (normal/hover/pressed/disabled/selected per icon-style-spec T-01~T-05)
- [X] T021 [P] [US3] Update interface context mockups (toolbar/sidebar with icons + app icon in window/taskbar context) under `docs/design/mockups/005-icon-set/` with `notes.md` recording design decisions
- [X] T022 [P] [US3] Integrate icon resources via `.qrc`: create or extend `src/ui/theme/theme.qrc` with `/perception/icons/` entries pointing to `src/ui/theme/icons/actions/*.png` and `src/ui/theme/icons/app/*`
- [X] T023 [US3] Wire window icon in `src/app/main.cpp`: add `mainWindow.setWindowIcon(QIcon(":/perception/icons/app/app-icon.ico"))` (or png fallback per T002)
- [X] T024 [US3] Run five-state conformance review per `specs/002-icon-design/contracts/conformance-checklist.md` section D + F (states distinguishable on dark theme, app icon 16px legible); record in checklist

**Story completion**: 五态评审通过；mockups 齐备；构建成功且窗口图标显示。

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 全量验收与文档收尾

- [X] T025 [P] Run full 31-item conformance review per `specs/002-icon-design/contracts/conformance-checklist.md` (all sections) — 0 non-conformances (SC-002)
- [X] T026 [P] Run regression: `cmake --build build --target Perception` + `ctest` full suite (no core breakage, quickstart.md scenario 4 & regression section)
- [X] T027 [P] Sync docs: reference icon spec from `docs/design/ui-guidelines.md` (design-system entry), confirm `docs/design/mockups/README.md` lists `005-icon-set/`, and mark all quickstart.md completion signals checked

---

## Dependencies

```text
Phase 1 Setup ──► Phase 2 Foundational (T004-T006) ──► US1 (T007-T009)
                                                          │
                                                          ▼
                                              US2 (T010-T019) ──► US3 (T020-T024)
                                                                    │
                                                                    ▼
                                                              Polish (T025-T027)
```

- **US1 (P1)** 依赖 Foundational 的 T004（规范定稿是 US1 评审对象）与 T006（校验脚本）。
- **US2 (P2)** 依赖 US1 的 T007（映射表先行，图标绘制后逐项登记校验）；六类图标绘制彼此独立。
- **US3 (P3)** 依赖 US2（mockups 需用成品图标）；T022 资源集成仅依赖 T017 位图产物。
- **Polish** 依赖全部用户故事完成。

### Parallel Opportunities

- Phase 1：T002、T003 可并行（不同文件/环境）。
- Phase 2：T005、T006 可并行（模板与脚本互不依赖）。
- Phase 3：T008 与 T009 可并行；T007 完成后 US2 即可启动。
- Phase 4：T010~T016 七条绘制/设计任务全部并行（六类图标 + 应用图标，文件互不重叠）；T019 盲测依赖全部图标完成后执行。
- Phase 5：T020、T021、T022 可并行；T023 依赖 T022。
- Phase 6：T025、T026、T027 可并行。

---

## Parallel Example: User Story 2

```bash
# 六类图标 + 应用图标并行绘制（[P] 任务）：
Task: "Draw 文件与数据 icons in src/ui/theme/icons/actions/file-*.svg"        # T010
Task: "Draw 编辑 icons in src/ui/theme/icons/actions/edit-*.svg"              # T011
Task: "Draw 视图与相机 icons in src/ui/theme/icons/actions/view-*.svg"        # T012
Task: "Draw 数据操作与分析 icons in src/ui/theme/icons/actions/analysis-*.svg" # T013
Task: "Draw 动画与播放 icons in src/ui/theme/icons/actions/animation-*.svg"    # T014
Task: "Draw 工具与设置 icons in src/ui/theme/icons/actions/tools-*.svg"        # T015
Task: "Design app icon src/ui/theme/icons/app/app-icon.svg"                   # T016
```

---

## Implementation Strategy

### MVP First（User Story 1）

1. Phase 1: Setup（T001–T003）
2. Phase 2: Foundational（T004–T006，CRITICAL 阻塞全部故事）
3. Phase 3: US1 规范定稿 + 映射表 + 校验脚本（T007–T009）
4. **STOP and VALIDATE**: 规范文档可逐项核验（US1 Independent Test），符合性清单基线建立
5. 规范评审通过后进入 US2

### Incremental Delivery

1. Setup + Foundational → 基础就绪
2. US1 规范 + 映射表 → 评审通过（**MVP 交付：规范可指导任意图标绘制**）
3. US2 图标集 + 应用图标 → 盲测通过 → Demo
4. US3 mockups + 资源集成 → 运行可见 → Demo
5. Polish 全量验收 → 0 不符合项

### Parallel Team Strategy

1. Team 完成 Setup + Foundational
2. Foundational 完成后：
   - Developer A: US1 规范/映射表
   - Developer B/C/D...: US2 六类图标并行（T010–T016）
   - Developer E: 应用图标（T016）
3. US2 图标集齐后，US3 mockups/资源集成并行推进

---

## Notes

- 图标全部原创自绘（SVG），禁止第三方图标库与网络资源（spec Assumptions）。
- 色板唯一来源 `docs/design/ui-guidelines.md` §3.1 Token，禁止新增色值（conformance C-1）。
- SVG 模板描边宽 2/2.5/3px、round cap/join、透明底；16px 档图形必须落在 12×12 安全区。
- 命名 kebab-case：`<功能区>-<功能>[-<变体>]`，功能区取 `file/edit/view/analysis/animation/tools`。
- [P] 任务 = 不同文件、无依赖；[Story] 标签映射用户故事。
- 每个逻辑组完成即提交；任一 checkpoint 可独立验证对应故事。
- 应用图标挂接仅设窗口图标（FR-011）；按钮 `QAction` 图标挂接属后续「按钮布局」需求。
- 若 T002 确认环境无 Qt5Svg：功能图标全部改预渲染 PNG（16/24/32 × 五态），应用图标用 `.ico`，更新 T017/T022/T023 资源引用。

# Tasks: Panel Layout Settings

**Input**: Design documents from `/specs/010-panel-layout-settings/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/panel-layout-config-api.md, contracts/panel-settings-dialog.md, quickstart.md

**Tests**: 本功能含测试任务。项目宪法 II（Test-First）强制纯逻辑层单测；spec 每 story 含 Independent Test；quickstart.md 定义 ctest 门禁与手动验证。测试任务先于实现（红-绿）。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- 单项目：`src/`、`tests/` 位于仓库根（`e:/spec-work/Perception/`）
- 本功能新代码落点：`src/ui/panellayout/`（纯 UI 变更，零新依赖，不触碰 `src/core`）
- 测试注册点：`tests/cpp/CMakeLists.txt`（GUI 测试在 `PERCEPTION_BUILD_GUI` + `Qt5Test_FOUND` 块内，仿 `subwindow_view_test`）

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 建立本功能模块目录

- [ ] T001 创建 `src/ui/panellayout/` 模块目录（本功能全部新文件落点：panel_layout_config / panel_preview_widget / panel_settings_dialog 的 .h/.cpp，plan.md Project Structure）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: `PanelLayoutConfig` 纯逻辑层（模式→区域映射 / 合法组合 / expand 决策 / 序列化），是所有 User Story 的单一事实源（contracts/panel-layout-config-api.md §1）

**⚠️ CRITICAL**: 本阶段完成前不得开始任何 User Story 实现

- [ ] T002 创建 `src/ui/panellayout/panel_layout_config.h`：定义 `PanelId {Data, Property, PyShell}`、`DockArea {Left, Right, Bottom}`、`PanelLayoutMode {DualOnly, DualWithConsole, DualReversedOnly, DualReversedWithConsole}`、`struct PanelLayoutConfig {mode; dataVisible; propertyVisible; consoleVisible}`（默认 `DualWithConsole` + 三显隐全 true），声明 `targetArea/modeDefaultsConsoleVisible`（FR-002/FR-003）、`isLegalArea`（FR-003）、`isPanelVisible`（FR-004/FR-005）、`isLegalConfig`（FR-008）、`modeToKey/modeFromKey`（FR-007），namespace `perception::ui`
- [ ] T003 创建 `tests/cpp/panel_layout_config_test.cpp`（红）：断言 FR-003 区域映射（四种模式的 Data/Property 左右身份 + PyShell 恒 Bottom + `isLegalArea` 拒绝 PyShell 侧边）、FR-008 组合恒合法（4 模式 × 8 显隐组合 `isLegalConfig` 全 true）、FR-005 expand 决策（`isPanelVisible` 随 cfg 正确输出三面板可见性）、FR-007 序列化 round-trip（modeToKey/modeFromKey，含未知 key 回退默认）；在 `tests/cpp/CMakeLists.txt` 注册 `panel_layout_config_test`（链接 `perception_ui`，仿 `layout_manager_test`）
- [ ] T004 实现 `src/ui/panellayout/panel_layout_config.cpp` 使 T003 测试通过（绿），并在 `src/ui/CMakeLists.txt` 登记 panellayout 源文件（规则见 contracts/panel-layout-config-api.md §2）

---

## Phase 3: User Story 1 (P1) - Open Panel Settings and Choose a Layout Mode

**Goal**: View 菜单入口打开 Panel Settings 对话框；四种模式选择并应用到主窗口；OK 持久化

**Independent Test**: 打开 View 菜单 → "Panel Settings"，对话框展示四种模式与三面板显隐开关；依次选择四种模式，Data/Property 左右互换、PyShell 底部显隐符合模式定义（spec US1 场景 1/2/3；quickstart 场景 1）

- [ ] T005 [P] [US1] 在 `src/ui/MainWindow.h` 声明 `void applyPanelLayout(const PanelLayoutConfig&)`（private）并在 `src/ui/MainWindow.cpp` 实现：`removeDockWidget` 三 dock → 按 `targetArea` 逐个 `addDockWidget` → 按 `isPanelVisible` `setVisible` → `resizeDocks` 规整（FR-003/FR-005；全屏态 `setContainerFullscreen` 期间调用需先退出或兼容处理）
- [ ] T006 [P] [US1] 创建 `src/ui/panellayout/panel_settings_dialog.{h,cpp}`：无边框自定义标题栏（复用 `src/ui/dialog_title_bar.h`）+ 4 个 checkable 模式按钮（`QButtonGroup` exclusive，objectName `panelModeButton`，QSS 样式名 `panelModeButton`）+ 3 个显隐 CheckBox（objectName `panelDataVisibleCheck` / `panelPropertyVisibleCheck` / `panelConsoleVisibleCheck`）+ 恢复默认（`panelResetDefaultsButton`）+ OK（`panelOkButton`）/ Cancel（`panelCancelButton`）；发射 `configChanged(PanelLayoutConfig)`（contracts/panel-settings-dialog.md §2/§3；FR-001/002/004）
- [ ] T007 [P] [US1] 创建 `src/ui/panellayout/panel_preview_widget.{h,cpp}`：自绘布局示意图（objectName `panelPreviewWidget`），几何复用 `PanelLayoutConfig::targetArea` / `isPanelVisible`，实时反映模式 × 显隐（左/右/底/中央区以主题色块表达，背景 `@viewBg@`、cell `@panelBg@`、边框 `@border@`；仿 `src/ui/subwindow/` 内 004 LayoutPreviewWidget 模式）（FR-006 对话框内示意）
- [ ] T008 [P] [US1] 在 `src/ui/MainWindow_assembly.cpp`：`createActions` 新增 `panelSettingsAction_`（复用 `view-panel-toggle` 图标，经 `makeActionIcon`）+ `createMenus` 在 View 菜单注册 "Panel Settings..."（置于 "Layout Settings..." 附近）+ 连接至 `openPanelSettings()` 槽（FR-001）
- [ ] T009 [US1] 在 `src/ui/MainWindow.h/.cpp` 实现 `openPanelSettings()`：以当前配置初始化对话框（含快照），连接 `configChanged` → `applyPanelLayout`，OK → 持久化 `panelSettings/mode`、`panelSettings/dataVisible`、`panelSettings/propertyVisible`、`panelSettings/consoleVisible` 四 key 并关闭（FR-007 写入；data-model.md §5；US1 场景 3）

---

## Phase 4: User Story 2 (P2) - Toggle Panel Visibility with Auto-Expand

**Goal**: 三面板独立显隐；隐藏后其余可见面板与中央区自动 expand（无空白死区、无重叠）

**Independent Test**: 取消/勾选三面板显隐，观察隐藏面板空间被其余面板与中央区按比例吸收；全隐 → 中央区占满；恢复显隐 → 面板回到模式定义区域（spec US2 场景 1/2/3；quickstart 场景 2）

- [ ] T010 [P] [US2] 在 `src/ui/theme/theme_template.qss` 增补 PanelSettingsDialog 控件 QSS：`panelModeButton:checked`（@accent@ 背景 + 文字）、三个显隐 CheckBox、预览区与按钮条，遵循 `docs/design/ui-guidelines.md` §4.1 覆盖矩阵（normal/hover/checked/disabled 全状态）
- [ ] T011 [US2] 创建 `tests/cpp/panel_settings_dialog_test.cpp`（QTest + `QSignalSpy`，`QApplication`）：打开对话框、四模式切换发 `configChanged` 且负载正确、三显隐 toggle 发信号、预览 widget 状态更新、OK 后 `QSettings` 四 key 写入、Cancel 后恢复快照（覆盖 US1/US2/US3 交互）；在 `tests/cpp/CMakeLists.txt` 的 `PERCEPTION_BUILD_GUI` + `Qt5Test_FOUND` 块注册（仿 `subwindow_view_test`；contracts/panel-settings-dialog.md §4）
- [ ] T012 [US2] 补强 `src/ui/MainWindow.cpp` `applyPanelLayout` expand 行为并验证：左面板隐 → 中央区横向扩展、底面板隐 → 中央区/侧面板纵向扩展、左右同隐/全隐 → 中央区占满可用区域（`resizeDocks` 比例分配，FR-005 场景 1/2/3；spec Edge Cases）

---

## Phase 5: User Story 3 (P2) - Live Preview Before Applying

**Goal**: 对话框内变更主窗口实时预览；Cancel/关闭回滚到打开前布局

**Independent Test**: 对话框打开时切换模式/显隐，主窗口立即更新；点击 Cancel 后主窗口恢复打开前布局（spec US3 场景 1/2/3；quickstart 场景 3）

- [ ] T013 [US3] 在 `src/ui/panellayout/panel_settings_dialog.{h,cpp}` 实现快照/回滚：构造时快照 `cfg0`（打开前配置），Cancel 或标题栏关闭按钮 → 恢复 `cfg0` 并 `configChanged(cfg0)`（经 MainWindow 重放 `applyPanelLayout`），OK 不触发回滚（US3 场景 3；contracts/panel-settings-dialog.md §3；SC-004 由 T011 断言）

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 持久化恢复、文档同步、全量回归

- [ ] T014 持久化恢复与重置联动：`src/ui/MainWindow.cpp` 构造后从 `QSettings` 读取 `panelSettings/*` 四 key 并 `applyPanelLayout`（SC-005 重启恢复）；`resetLayout()` 同步清除四 key 并应用默认配置（FR-007；data-model.md §5；plan §5）
- [ ] T015 更新 `docs/design/ui-guidelines.md`（§4.7 增补 Panel Settings 弹窗控件规范：四模式按钮组、显隐 CheckBox、快照/回滚语义）与 `README.md`（功能清单与 "View → Panel Settings" 入口说明）（宪法文档同步要求）
- [ ] T016 全量回归：完整构建（CMake 配置同步）+ `ctest -R panel_layout_config_test` + `ctest -R panel_settings_dialog_test` + 全量 `ctest` 通过 + quickstart.md 手动场景 1-5 逐条验证（SC-001~SC-005；宪法质量门禁）

---

## Dependencies

```
Phase 1 (T001)
  └─► Phase 2 (T002 → T003 → T004)          # PanelLayoutConfig 纯逻辑层
        └─► US1 (T005-T009)                  # 入口 + 对话框 + 应用层 + OK 持久化
        │      ├─ T005 [P] applyPanelLayout ──► T012 (US2 expand 补强)
        │      ├─ T006 [P] 对话框          ──► T011 (US2 GUI 测试) / T013 (US3 回滚)
        │      ├─ T007 [P] 预览 widget     ──► T011
        │      └─ T008 [P] 菜单动作        ──► T009 (openPanelSettings 接线)
        └─► US2 (T010-T012)                  # QSS + GUI 测试 + expand 补强
              └─► US3 (T013)                 # 快照/回滚（依赖 T009/T006）
                    └─► Polish (T014-T016)   # 持久化恢复 + 文档 + 回归
```

- US2 的 T012 依赖 US1 的 T005（applyPanelLayout 已含基础 expand）；US3 的 T013 依赖 US1 的 T006/T009（对话框与 openPanelSettings 接线）；其余 US 间无跨依赖，可在 Foundational 完成后由不同成员并行推进。

## Parallel Example: 各 User Story

```bash
# US1（Foundational 完成后，四项并行）：
Task: "T005 实现 applyPanelLayout（src/ui/MainWindow.cpp）"
Task: "T006 创建 PanelSettingsDialog（src/ui/panellayout/panel_settings_dialog.{h,cpp}）"
Task: "T007 创建 PanelPreviewWidget（src/ui/panellayout/panel_preview_widget.{h,cpp}）"
Task: "T008 新增 View 菜单动作（src/ui/MainWindow_assembly.cpp）"

# US2（两项并行）：
Task: "T010 QSS 增补（src/ui/theme/theme_template.qss）"
Task: "T011 GUI 交互测试（tests/cpp/panel_settings_dialog_test.cpp）"

# US3（独立）：
Task: "T013 快照/回滚（src/ui/panellayout/panel_settings_dialog.{h,cpp}）"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1: T001 建目录
2. Phase 2: T002-T004 `PanelLayoutConfig` 纯逻辑层（红-绿，单测通过）
3. Phase 3: T005-T009 US1 → 菜单入口 + 对话框 + 四模式应用 + OK 持久化
4. **STOP and VALIDATE**: quickstart 场景 1/4（模式切换 + 持久化）独立验证
5. MVP = US1（入口 + 四模式），可交付演示

### Incremental Delivery

1. Setup + Foundational → 纯逻辑层就绪（ctest `panel_layout_config_test` 绿）
2. US1 → 模式选择 + 应用 + 持久化（MVP）
3. US2 → 显隐 toggle + expand + QSS + GUI 测试
4. US3 → 实时预览 + Cancel 回滚
5. Polish → 重启恢复 + resetLayout 联动 + 文档 + 全量回归

### Parallel Team Strategy

- 完成 Phase 2 后：Developer A 做 US1（T005-T009），Developer B 待 US1 的 T005/T006 落地后做 US2（T010-T012），Developer C 做 US3（T013，依赖 US1 接线完成）
- US1 内部 T005/T006/T007/T008 四文件互不冲突，可并行

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- 每个 story 独立完成、独立测试（spec Independent Test + quickstart 场景）
- 测试先于实现（T003/T011 为红，T004/T005-T009 实现后转绿）
- 禁止触碰 `src/core`、`src/render`、`src/python`（宪法 Layered Core / Scope）；零新依赖（research §1）
- 弹窗 MUST 无边框自定义标题栏（`dialog_title_bar`）；复用 `view-panel-*` 图标族（research §7）
- 提交按任务或逻辑组进行；任一 checkpoint 可停下独立验证 story

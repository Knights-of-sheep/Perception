# Tasks: 子窗口布局管理

**Input**: Design documents from `/specs/004-dock-layout-manager/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: 本项目宪法 II 强制 Test-First（红-绿-重构）。布局计算类 `LayoutManager` 配置 CTest 单测（`tests/cpp/layout_manager_test.cpp`，先写后失败再实现）；其余 UI 交互按 `quickstart.md` 手动验证 + 截图比对。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- 本次改动集中在 `src/ui/subwindow/`（新增）、`src/ui/`（接入）、`src/ui/console/`（Python 桥）、`src/ui/theme/`（QSS）、`tests/cpp/`（单测）

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 将新增模块接入现有构建与测试体系

- [ ] T001 将 `src/ui/subwindow/` 新增源文件（subwindow_container.cpp、subwindow_view.cpp、layout_manager.cpp、layout_settings_dialog.cpp）加入 `src/ui/CMakeLists.txt` 的 `perception_ui` 源列表（AUTOMOC/AUTORCC 已开启）
- [ ] T002 [P] 在 `tests/cpp/CMakeLists.txt` 注册 `layout_manager_test` 可执行目标并 `add_test`（链接 `perception_ui`，参照现有 `icon_action_map_test` 先例；布局测试仅用 QtCore 类型，无需 GUI 平台插件）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: 子窗口、容器与纯布局计算——所有用户故事的前提

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T003 编写 `tests/cpp/layout_manager_test.cpp` 测试（**红：先失败**）——覆盖：按行/按列/网格行列数（FR-004~006）、最大列/行约束 N=1..10（FR-007/008、SC-003）、容量不足时行数突破且全部可访问（FR-014）、相同宽高 cell 尺寸一致（FR-009、SC-004）、相邻间隙 ≥4px 一致（FR-015、SC-009）、windowCount=0 返回空网格
- [ ] T004 实现 `src/ui/subwindow/layout_manager.{h,cpp}`（**绿**）——`LayoutMode`/`LayoutConfig`/`GridLayout` 与 `computeGrid`/`cellSize`/`cellRect`，接口严格按 `contracts/layout-manager-api.md` 第 1 节；容量语义按 `research.md` 第 6 节
- [ ] T005 [P] 实现 `src/ui/subwindow/subwindow_view.{h,cpp}` —— 普通 QWidget 子控件（无系统标题栏，FR-018）、顶部自定义标题区（标题 + 最大化/全屏/关闭按钮 + 双击信号，交互复用 `MainWindow.cpp` 内 `DockTitleBar` 模式）、内容区为占位渲染视图（FR-003）、单击任意区域发射选中信号（spec Assumptions）
- [ ] T006 [P] 实现 `src/ui/subwindow/subwindow_container.{h,cpp}` —— QWidget 容器：`addSubwindow`/`removeSubwindow`、`setLayoutConfig`（用 QGridLayout + `setSpacing(4)` 执行重排，FR-011/015）、`setMaximized`/`exitMaximized` 骨架（FR-016）、单子窗口铺满可用范围（Edge Cases）、窗口过小可用滚动兜底
- [ ] T007 将 `SubwindowContainer` 接入主窗口中央区域：`src/ui/MainWindow.cpp` 的 `createCentralArea()` 用容器替换 `centralPlaceholder_`（成员新增于 `src/ui/MainWindow.h`；无子窗口时保留空状态提示）

**Checkpoint**: Foundation ready - 布局计算单测全绿（`ctest -R layout_manager_test`），子窗口/容器可实例化

---

## Phase 3: User Story 1 - 创建子窗口：Python 命令与菜单双入口 (Priority: P1) 🎯 MVP

**Goal**: 用户可经 PyShell Python 命令 `create_window(name)` 与菜单"新建子窗口"两种入口创建渲染子窗口，创建后立即参与当前布局

**Independent Test**: quickstart 3.1 —— 菜单创建 ×3 + `create_window("曲线图")` ×2，5 个子窗口 1 秒内出现（SC-007）、两入口行为一致（SC-008）

### Implementation for User Story 1

- [ ] T008 [P] [US1] 在 `src/ui/console/PythonConsole.cpp` 注入全局函数 `create_window`（复用 `_cpp_log` 的 `PyMethodDef` + `PyCFunction_New` 模式，`PythonConsole.cpp:115-134`），参数校验（空名/非 str 返回 False 不崩溃）按 `contracts/python-create-window.md`；经 `std::function` 桥接声明于 `src/ui/MainWindow.h`
- [ ] T009 [P] [US1] 在 `src/ui/MainWindow.cpp` 的 `createActions()`/`createMenus()` 添加"视图 → 新建子窗口"动作，复用 `icon-map.yaml` 现有 `view-multi-view` 图标（`action_icon_map` 接入）
- [ ] T010 [US1] 实现 `MainWindow::createSubwindow(const QString& title)`：创建 `SubwindowView`、加入容器并立即重排（FR-001）；菜单与 Python 桥共用此入口保证行为一致（FR-002、SC-008）

**Checkpoint**: US1 独立可用——双入口创建、占位内容不崩溃（FR-003）

---

## Phase 4: User Story 2 - 一键切换排列模式：按行 / 按列 / 网格 (Priority: P1)

**Goal**: 布局设置入口中选择按行/按列/网格，已打开子窗口立即重排，渲染内容与视图状态不丢失

**Independent Test**: quickstart 3.2 —— ≥2 个子窗口依次切换三模式，截图比对：互不重叠、间隙一致 ≥4px（SC-009）、状态保留（FR-013）

### Implementation for User Story 2

- [ ] T011 [P] [US2] 创建 `src/ui/subwindow/layout_settings_dialog.{h,cpp}` 骨架：排列模式三选一控件（按行/按列/网格），修改即发射配置变更信号（不等待确认，FR-011 精神）
- [ ] T012 [US2] 在 `src/ui/MainWindow.cpp` 添加"视图 → 布局…"菜单动作打开对话框，将模式变更转发至 `SubwindowContainer::setLayoutConfig` 立即重排（FR-004~006/011）；切换过程子窗口仅 hide/show 重排，不销毁（FR-013）

**Checkpoint**: US1 + US2 构成完整可演示 MVP——创建 + 三模式排列

---

## Phase 5: User Story 3 - 设置最大行 / 最大列约束 (Priority: P1)

**Goal**: 网格模式下设置最大列数/最大行数（1–10），自动换行/换列；容量不足时子窗口不丢失、可访问

**Independent Test**: quickstart 3.3 —— 5 个子窗口、最大列=2 → 3 行 2 列（SC-003）；最大行=最大列=2 → 第 5 个可访问（FR-014）；非法输入回退（FR-012）

### Implementation for User Story 3

- [ ] T013 [P] [US3] 在 `src/ui/subwindow/layout_settings_dialog.cpp` 增加"最大列数/最大行数"SpinBox（0–10，0=自动/不限；网格模式下启用）；非法输入（0 或负数或非数字）显示明确提示并恢复上次有效值（FR-012）
- [ ] T014 [US3] 接线：对话框约束变更复用 T012 的转发路径 → `SubwindowContainer::setLayoutConfig`（`computeGrid` 已实现约束语义）重排（FR-007/008/014）

**Checkpoint**: P1 三故事全部独立可用——创建、排列、约束闭环

---

## Phase 6: User Story 4 - 保持相同宽高 (Priority: P2)

**Goal**: 开启后同一排列全部子窗口宽高统一且随窗口等比缩放；关闭后恢复各自独立尺寸

**Independent Test**: quickstart 3.4 —— 开启后尺寸差异 <1px（SC-004）、拉伸同步等比（FR-009）；关闭恢复开启前独立尺寸（FR-010）；间隙保持（Edge Cases）

### Implementation for User Story 4

- [ ] T015 [P] [US4] 在 `src/ui/subwindow/layout_settings_dialog.cpp` 增加"保持相同宽高"CheckBox，变更即生效（FR-009/011）
- [ ] T016 [US4] 在 `src/ui/subwindow/subwindow_view.cpp` 记录开启前独立尺寸（`sizeBeforeSameSize`，data-model.md 实体字段），开启时容器统一 cell 尺寸（`LayoutManager::cellSize`，FR-009），关闭时恢复记忆尺寸（FR-010）；任意模式可叠加（US4 Why this priority）

**Checkpoint**: US4 独立可测——任意模式下开关生效、窗口拉伸同步等比

---

## Phase 7: User Story 5 - 统一布局设置入口与状态反馈 (Priority: P2)

**Goal**: 布局设置界面完整展示当前排列模式/最大行/最大列/相同宽高取值，修改即时生效，任意排列状态下入口一键可达

**Independent Test**: quickstart —— 打开设置界面回显当前值；逐项修改立即生效无确认；深/浅主题下控件清晰（US5 场景 3）；子窗口杂乱时 View 菜单入口可达

### Implementation for User Story 5

- [ ] T017 [P] [US5] 对话框构造时注入当前 `LayoutConfig` 并回显全部控件当前值（`src/ui/subwindow/layout_settings_dialog.cpp`，FR-011 场景 1）
- [ ] T018 [US5] 深/浅主题适配检查与微调（`src/ui/theme/theme_template.qss` 增补对话框/标题区/选中高亮样式，沿用现有主题体系）——UI 改动对照 `docs/design/mockups/005-icon-set/main-window-mockup.png` 风格；确认 View 菜单固定入口在排列混乱时可达（US5 场景 2）

**Checkpoint**: US5 完成——统一设置入口 + 全量状态回显 + 即时生效

---

## Phase 8: User Story 6 - 子窗口最大化与全屏 (Priority: P2)

**Goal**: 选中子窗口最大化（占满中央区域，其余隐藏）或全屏（占满主窗口工作区，面板临时隐藏）；退出后原布局与面板显隐完整恢复；全程无系统标题栏

**Independent Test**: quickstart 3.5 —— 选中 A 触发最大化/全屏并截图比对覆盖范围（SC-010/011）；退出恢复布局与面板（FR-017）；内容与视图状态不丢失（FR-019）；三状态无系统标题栏（SC-012）

### Implementation for User Story 6

- [ ] T019 [P] [US6] 在 `src/ui/subwindow/subwindow_view.cpp` 完成标题区触发入口（research.md 第 3 节）：最大化/全屏/关闭按钮、双击标题区=最大化、右键菜单；选中时高亮；均发射对应信号
- [ ] T020 [US6] 在 `src/ui/subwindow/subwindow_container.cpp` 实现 `setMaximized`/`exitMaximized`：最大化时仅显示选中子窗口并填满容器（其余 hide），退出恢复原排列（FR-016）；子窗口不销毁、内容与视图状态保留（FR-019）
- [ ] T021 [US6] 在 `src/ui/MainWindow.cpp` 实现全屏协调：选中子窗口 re-parent 至主窗口并覆盖整个工作区，经 `saveState`/`restoreState`（或逐 dock toggle）临时隐藏属性/数据/PyShell 面板；`Esc` 退出；退出后 re-parent 回容器、面板与布局完整恢复（FR-017/019）；普通/最大化/全屏三状态均无系统标题栏（FR-018、SC-012）

**Checkpoint**: US6 完成——最大化/全屏进出闭环，布局与面板完整恢复

---

## Phase 9: Polish & Cross-Cutting Concerns

**Purpose**: 跨故事边界场景加固与整体验证

- [ ] T022 [P] Edge Cases 加固（`src/ui/subwindow/subwindow_container.cpp`）：0/1 子窗口正常呈现不报错、20+ 子窗口排列无明显性能劣化（SC-005 连续 50 次操作）、窗口过小时优先最小可用尺寸并可滚动（spec Edge Cases）
- [ ] T023 [P] 高 DPI 验证与微调：系统缩放 150%/200% 下布局不错位、子窗口边界与控件清晰（SC-006，`src/ui/theme/theme_template.qss` 与布局相关微调）
- [ ] T024 [P] 按 `quickstart.md` 全场景走查并截图比对 mockups（SC-001/002/005/006/007/008/009/010/011/012），修复发现的差异；更新 `docs/` 相关文档

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - US3/US4/US5 依赖前置故事（见下），US6 仅依赖 Foundational 可在 P1 故事后并行
  - 单人多角色按优先级顺序执行（P1 → P2）
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 3 (P1)**: Depends on US2（复用 T011 对话框骨架）
- **User Story 4 (P2)**: Depends on US2/US3（排列与约束基础）
- **User Story 5 (P2)**: Depends on US2/US3/US4（回显全部配置项）
- **User Story 6 (P2)**: Depends on Foundational only（选中/容器在 T005/T006）；与 US3/4/5 可并行

### Within Each User Story

- 布局计算测试（T003）MUST 先失败再实现（T004）
- 子窗口/容器（T005/T006）先于 MainWindow 接入（T007）
- 核心实现（T010）先于集成（T012/T014/T016/T021）
- Story complete before moving to next priority

### Parallel Opportunities

- T002 可与 T001 并行（Setup）
- T005/T006 可并行（不同文件）；T003→T004 串行（红-绿）
- Foundational 完成后：US1（T008/T009 并行）与 US2（T011）可并行
- US3~US6 各故事内部标记 [P] 的任务可并行
- 不同用户故事可由不同成员并行（US1、US2、US6 互不阻塞）

---

## Parallel Example: User Story 1

```bash
# 并行启动 US1 的两个互不依赖任务：
Task: "T008 注入 create_window Python 桥接 in src/ui/console/PythonConsole.cpp"
Task: "T009 添加'视图 → 新建子窗口'菜单动作 in src/ui/MainWindow.cpp"

# 依赖两者完成后串行：
Task: "T010 实现 MainWindow::createSubwindow 并接线双入口"
```

## Parallel Example: Foundational

```bash
# 并行启动布局计算与 UI 组件：
Task: "T003 编写 layout_manager_test.cpp（红）→ T004 实现 LayoutManager（绿）"
Task: "T005 实现 SubwindowView in src/ui/subwindow/subwindow_view.{h,cpp}"
Task: "T006 实现 SubwindowContainer in src/ui/subwindow/subwindow_container.{h,cpp}"

# 完成后串行：
Task: "T007 MainWindow 接入 SubwindowContainer"
```

---

## Implementation Strategy

### MVP First (User Story 1 + 2)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational（阻塞一切）
3. Complete Phase 3: User Story 1（创建入口）
4. Complete Phase 4: User Story 2（排列模式切换）
5. **STOP and VALIDATE**: 创建 + 三模式切换独立验证（quickstart 3.1/3.2）
6. Demo/MVP 交付（spec：US2 单独即可构成可演示 MVP；加入 US1 后闭环）

### Incremental Delivery

1. Complete Setup + Foundational → 布局引擎与子窗口基础就绪
2. Add User Story 1 → 可创建子窗口 → 验证（SC-007/008）
3. Add User Story 2 → 三模式排列 → Demo（MVP）
4. Add User Story 3 → 最大行/列约束 → 验证（SC-003、FR-014）
5. Add User Story 4 → 相同宽高 → 验证（SC-004、FR-010）
6. Add User Story 5 → 统一设置入口回显 → 验证（FR-011）
7. Add User Story 6 → 最大化/全屏 → 验证（SC-010/011/012）
8. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1（T008/T009 并行）
   - Developer B: User Story 2 → 3 → 4 → 5（对话框演进主线）
   - Developer C: User Story 6（标题区交互 + 全屏协调）
3. Stories complete and integrate independently（US6 与 US3/4/5 不冲突）

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable（quickstart 对应章节）
- Verify tests fail before implementing（T003 红 → T004 绿）
- Commit after each task or logical group（/speckit.git.commit）
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
- 契约引用：`contracts/layout-manager-api.md`（LayoutManager 接口）、`contracts/python-create-window.md`（create_window 校验）；计算语义见 `data-model.md` 第 3 节

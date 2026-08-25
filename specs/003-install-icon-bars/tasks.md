---

description: "Task list for installing program icon and function icon bars"
---

# Tasks: 安装程序图标与功能图标

**Input**: Design documents from `/specs/003-install-icon-bars/`

**Prerequisites**: plan.md, spec.md (user stories), research.md, data-model.md, contracts/icon-action-map.md, quickstart.md

**Tests**: 宪法 V 要求测试先行 → 本功能含契约单测（`tests/cpp/icon_action_map_test.cpp`，先红后绿）。

**Organization**: Tasks are grouped by user story (spec.md US1~US4) to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1=菜单图标, US2=左侧功能栏, US3=右侧功能栏, US4=应用图标
- Include exact file paths in descriptions

## 关键约定（来自 research.md / contracts/icon-action-map.md）

- 图标资源根：`src/ui/theme/icons/`（SVG 源 `actions/`、PNG `png/actions/`、语义表 `icon-map.yaml`、注册表 `src/ui/theme/theme.qrc`）
- 动作-图标唯一契约：`contracts/icon-action-map.md`（§1 菜单、§2 左栏 10、§3 右栏 9、§4 新增 4 图标、§5 五态）
- 五态：normal/hover/pressed 由 QSS 容器（theme_template.qss）+ 原始 PNG 实现；disabled/selected 由 IconFactory 运行时染色派生（T-04/T-06）
- 未实现功能（撤销/重做/加载脚本/视频录制/刷新 + 右栏全部 9 枚）一律 `QAction::setEnabled(false)` + tooltip 注明，不连槽（FR-011）

---

## Phase 1: Setup（共享图标资源基础设施）

**Purpose**: 新增 4 枚缺口图标（SC-006）并接入既有图标管线（render→登记→qrc→校验）。

- [X] T001 [P] 新建图标 `file-load-script`（加载脚本语义，遵循 icon-style-spec：S-01 实心填充 FG_TEXT、G-01 16 网格 12×12 安全区）：`src/ui/theme/icons/actions/file-load-script.svg`
- [X] T002 [P] 新建图标 `file-record-screen`（主界面视频录制语义）：`src/ui/theme/icons/actions/file-record-screen.svg`
- [X] T003 [P] 新建图标 `view-refresh`（刷新语义）：`src/ui/theme/icons/actions/view-refresh.svg`
- [X] T004 [P] 新建图标 `view-panel-toggle`（面板显隐通用语义）：`src/ui/theme/icons/actions/view-panel-toggle.svg`
- [X] T005 渲染/登记/注册：运行 `scripts/render_icons.py` 生成 4 枚图标 PNG（16/24/32 三档到 `src/ui/theme/icons/png/actions/`）；在 `src/ui/theme/icons/icon-map.yaml` 登记 4 条语义；运行 `scripts/gen_qrc.py` 重新生成 `src/ui/theme/theme.qrc`（依赖 T001-T004）
- [X] T006 符合性校验：运行 `scripts/check_icons.py`，确认新增 4 枚图标 0 不符合（SC-006，依赖 T005）

---

## Phase 2: Foundational（动作-图标挂接基础设施）

**Purpose**: 阻塞全部用户故事的共享机制——ActionIconMap 契约表、IconFactory 五态派生、功能栏 QSS、契约单测。

**⚠️ CRITICAL**: 本阶段完成后用户故事才可开始（US4 除外，见依赖）。

- [X] T007 创建 ActionIconMap 常量表：`src/ui/action_icon_map.h` / `src/ui/action_icon_map.cpp`，严格按 `contracts/icon-action-map.md` §1~§3 定义全部动作的 id/text/tooltip/iconId/checkable/enabled（契约唯一来源，供 MainWindow 与单测共用）
- [X] T008 [P] 创建 IconFactory 五态派生：`src/ui/theme/icon_factory.h` / `src/ui/theme/icon_factory.cpp`，对同一 PNG 运行时派生 disabled（FG_TEXT_DISABLED #6E6E6E + 40% 透明度）与 selected（ACCENT 着色）变体并注册为 QIcon 的 Disabled/On 状态（满足 T-07「规范可派生」）
- [X] T009 [P] 扩展功能栏 QSS：在 `src/ui/theme/theme_template.qss` 为 `#leftToolBar` / `#rightToolBar` 定义纵向样式（ToolButtonIconOnly、iconSize 24px、五态容器：hover=BG_CONTROL 圆角底、pressed=加深、checked=SELECTION_BG 底；兼容 15 主题 Token 机制）
- [X] T010 编写契约单测：`tests/cpp/icon_action_map_test.cpp` 并注册到 `tests/cpp/CMakeLists.txt`（add_executable + add_test），断言 data-model V-1~V-8（左栏 10 集合固定、右栏 9 集合固定、菜单操作项 iconId 非空、tooltip 非空、图标 PNG 三档存在、未实现动作 enabled=false、面板开关 checkable、左栏可用按钮与菜单同动作）——**先红后绿**（依赖 T007）
- [X] T011 构建接线：更新 `src/ui/CMakeLists.txt`，将 `action_icon_map.cpp` 与 `theme/icon_factory.cpp` 加入 perception_ui 源码列表（依赖 T007、T008）

**Checkpoint**: Foundation ready —— ActionIconMap/IconFactory/QSS/单测齐备，用户故事可开始。

---

## Phase 3: User Story 1 - 菜单项配图标，操作一眼可辨 (Priority: P1) 🎯 MVP

**Goal**: 全部菜单操作项配置语义图标（FR-002），新增 5 个禁用态动作（FR-011）。

**Independent Test**: 展开文件/编辑/视图/帮助/设置菜单逐项核对图标，对照 `contracts/icon-action-map.md` 抽查 ≥10 项语义一致；`icon_action_map_test` V-3 全绿；`--snapshot` 快照可见菜单图标（SC-001/SC-004）。

- [X] T012 [US1] 新增禁用动作：在 `src/ui/MainWindow.h` 声明 undo/redo/loadScript/recordScreen/refresh 5 个 `QAction*` 成员，在 `src/ui/MainWindow.cpp` 的 `MainWindow::createActions()` 创建并 `setEnabled(false)` + tooltip 注明「功能即将推出」（FR-011）；撤销/重做加入编辑菜单
- [X] T013 [US1] 菜单图标挂接：在 `src/ui/MainWindow.cpp` `createActions()`/`createMenus()` 为全部菜单操作项设置图标（openAction_/exportAction_/exportImageAction_/exitAction_/toggleFileDockAction_/togglePropertyDockAction_/togglePythonConsoleAction_/resetLayoutAction_/vtkLogAction_/openLogDirAction_/setLogPathAction_/clearLogAction_/aboutAction_ 等），经 ActionIconMap + IconFactory；主题/日志级别状态列表不配图标（research 决策 1）
- [X] T014 [US1] 验证：运行 `icon_action_map_test`（V-3 菜单 iconId 非空）+ 构建 GUI 后 `--snapshot` 抓菜单栏截图，抽查 ≥10 项图标与契约语义一致（SC-001/SC-004/SC-005 缩放清晰）

**Checkpoint**: US1 完成——菜单图标化可作为独立增量交付。

---

## Phase 4: User Story 2 - 左侧功能栏：通用功能一键可达 (Priority: P1)

**Goal**: 左侧纵向功能栏 10 个带图标按钮（FR-003）；5 个已存在功能按钮接线与菜单行为一致（FR-004），5 个未实现功能按钮禁用占位（FR-011）；悬停中文提示（FR-006）。

**Independent Test**: 左栏 10 按钮存在且图标齐全；点击「加载文件」「主界面截图」与菜单行为一致；点击三个面板开关按钮，面板显隐且按钮选中态反映可见性（FR-003 场景 3）；点击 5 个禁用按钮无副作用；悬停 0.5s 显示中文 tooltip；`icon_action_map_test` V-1/V-4/V-6/V-8 全绿。

- [X] T015 [US2] 结构声明：在 `src/ui/MainWindow.h` 声明 `void createToolbars()` 与 `QToolBar* leftToolBar_` / `QToolBar* rightToolBar_` 成员，在 `src/ui/MainWindow.cpp` 构造函数中调用 `createToolbars()`
- [X] T016 [US2] 实现左栏：在 `src/ui/MainWindow.cpp` `MainWindow::createToolbars()` 创建左侧 `QToolBar`（Qt::LeftToolBarArea、`setOrientation(Qt::Vertical)`、objectName `leftToolBar`），按契约 §2 顺序组装 10 按钮：undo/redo/loadScript/recordScreen/refresh 为禁用态（复用 T012 动作），openFile/exportImage 与菜单同动作接线，三面板开关复用 toggle×3 动作（FR-003/004/011）
- [X] T017 [US2] 面板状态同步：在 `src/ui/MainWindow.cpp` 实现三个面板开关动作的 checkable 选中态与 dock 可见性联动（toggleFileDockAction_/togglePropertyDockAction_/togglePythonConsoleAction_ 的 checked 始终反映对应 dock 显示状态，FR-003 场景 3）
- [X] T018 [US2] 验证：运行 `icon_action_map_test`（V-1/V-4/V-6/V-8）+ 手动交互（接线按钮行为一致、禁用按钮无副作用、tooltip 中文、150%/200% 缩放图标清晰不错位 FR-010）

**Checkpoint**: US1 + US2 完成——左侧功能栏交付。

---

## Phase 5: User Story 3 - 右侧功能栏：领域功能按钮 (Priority: P2)

**Goal**: 右侧纵向功能栏 9 个带图标按钮（FR-005）；对应功能未实现，全部禁用态占位（FR-011）；悬停中文提示（FR-006）。

**Independent Test**: 右栏 9 按钮存在且图标齐全（视图 4 + 数据 5）；点击任一按钮为禁用态无副作用、不崩溃；悬停显示中文 tooltip；恢复默认布局后功能栏复位（FR-009）；`icon_action_map_test` V-2 全绿。

- [X] T019 [US3] 实现右栏：在 `src/ui/MainWindow.cpp` `createToolbars()` 中扩展右侧 `QToolBar`（Qt::RightToolBarArea、`setOrientation(Qt::Vertical)`、objectName `rightToolBar`），按契约 §3 顺序组装 9 个禁用态动作（view-zoom-in/zoom-out/fit-screen/reset-camera + analysis-curve-add/remove/extract/axis-settings/legend，经 ActionIconMap 取图标与 tooltip 并 `setEnabled(false)`，tooltip 注明「功能待实现」）（FR-005/011）
- [X] T020 [US3] 验证：运行 `icon_action_map_test`（V-2 右栏集合固定）+ 构建后 `--snapshot` 快照含右栏 + 手动交互（点击无副作用、tooltip、视图菜单「重置布局」后右栏回到右侧原位 FR-009）

**Checkpoint**: US1 + US2 + US3 全部完成。

---

## Phase 6: User Story 4 - 应用图标统一呈现 (Priority: P2)

**Goal**: 窗口标题栏、任务栏、Alt+Tab 与资源管理器 exe 图标均显示设计版应用图标（FR-001）；多尺寸清晰（SC 200% 缩放）。

**Independent Test**: 构建后在资源管理器查看 exe 文件图标 = 设计版 app-icon；启动程序窗口标题栏/任务栏/Alt+Tab 均显示 app-icon；200% 缩放任务栏清晰可辨。**与 US1~US3 完全并行**（仅触碰 `src/app/`，不依赖 Setup/Foundational 新增内容）。

- [X] T021 [US4] exe 资源图标：新增 `src/app/app.rc` 嵌入 `src/ui/theme/icons/app/app-icon.ico`（002 已交付，7 分辨率 ICO），并在 `src/app/CMakeLists.txt` 改为 `add_executable(perception main.cpp app.rc)`（保留 CONSOLE 子系统与 Python 链接选项注释，勿改 WIN32_EXECUTABLE）
- [X] T022 [US4] 集成验证：确认 `src/app/main.cpp` 已有 `setWindowIcon` 挂接（002 已实现，无需改动）；构建后核对资源管理器 exe 图标、窗口标题栏、任务栏、Alt+Tab 呈现（FR-001），200% 缩放图标清晰

**Checkpoint**: 四个用户故事全部完成。

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: 跨故事收尾——宪法 II mockup 自查、FR-008 缺失回退、全量回归、验收。

- [X] T023 [P] 功能栏 mockup 自查：按 plan 宪法 II 缓解措施，生成左右功能栏视觉稿（make_mockups.py 或既有 mockups 管线）到 `docs/design/mockups/`，实现后截图与 mockup 对比自查
- [X] T024 图标缺失回退：在 ActionIconMap/IconFactory 挂接处捕获图标资源缺失（FR-008），回退文字/占位显示并记录错误日志，程序不崩溃（改 `src/ui/` 相关源文件）
- [X] T025 全量回归：运行 `scripts/build.ps1 -Gui -UnitTests -Pytest`，ctest 与 pytest 全绿；启动目检功能栏图标就绪 <1s、无缺失闪烁（SC-002）
- [X] T026 [P] 验收：按 `specs/003-install-icon-bars/quickstart.md` 场景 1~5 逐项执行并勾选完成信号表（图标管线 / 构建+单测 / 快照 / 交互 9 项 / 回归）

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: 无依赖，T001-T004 并行创建 SVG → T005 渲染/登记/注册 → T006 校验
- **Foundational (Phase 2)**: 依赖 Setup —— **阻塞 US1/US2/US3**
- **US1 (Phase 3)**: 依赖 Setup + Foundational（MVP）
- **US2 (Phase 4)**: 依赖 Setup + Foundational + US1（复用 T012 动作与菜单接线模式）
- **US3 (Phase 5)**: 依赖 Setup + Foundational + US2（共享 `createToolbars()` 与 `MainWindow.h` 成员）
- **US4 (Phase 6)**: 独立（仅 `src/app/`，无跨故事依赖），可与 US1~US3 全程并行
- **Polish (Phase 7)**: 依赖全部用户故事完成

### User Story Dependencies

- **US1 (P1)**: 可独立实现与测试（菜单图标，最小增量）
- **US2 (P1)**: 依赖 US1 的动作基建；因共享 `src/ui/MainWindow.h/.cpp` 顺序执行
- **US3 (P2)**: 依赖 US2 的 `createToolbars()` 结构；因共享同一文件顺序执行
- **US4 (P2)**: 与 US1~US3 完全并行（不同文件），可安排给并行开发者

### Within Each User Story

- 契约单测（T010）先行编写并确认红 → 实现后转绿
- 基础设施（ActionIconMap）→ 动作 → 界面组装 → 验证

### Parallel Opportunities

- **Setup**: T001/T002/T003/T004 四枚 SVG 并行创建
- **Foundational**: T008（IconFactory）与 T009（QSS）并行；T010 依赖 T007
- **US4**: 整条 story 与 US1~US3 并行（不同文件 `src/app/`）
- **Polish**: T023 与 T026 并行
- 团队扩容时：A 完成 US1→US2→US3（MainWindow 顺序），B 并行完成 US4 与 Setup 图标

---

## Parallel Example: 并行执行示例

```bash
# Setup 阶段：四枚新图标同时创建（互不依赖）
Task: "T001 新建 file-load-script.svg（src/ui/theme/icons/actions/）"
Task: "T002 新建 file-record-screen.svg（src/ui/theme/icons/actions/）"
Task: "T003 新建 view-refresh.svg（src/ui/theme/icons/actions/）"
Task: "T004 新建 view-panel-toggle.svg（src/ui/theme/icons/actions/）"

# Foundational 阶段：五态派生与 QSS 样式并行
Task: "T008 创建 IconFactory（src/ui/theme/icon_factory.h/.cpp）"
Task: "T009 扩展 theme_template.qss 功能栏样式"

# US4 与 MainWindow 系列故事并行（不同文件，无冲突）
Task: "T021 新增 src/app/app.rc 嵌入 exe 图标"
Task: "T012 新增 5 个禁用动作（src/ui/MainWindow.cpp createActions()）"
```

---

## Implementation Strategy

### MVP First（Phase 3 = User Story 1）

1. 完成 Phase 1：Setup（4 枚新图标 + 管线）
2. 完成 Phase 2：Foundational（ActionIconMap + IconFactory + QSS + 契约单测，CRITICAL）
3. 完成 Phase 3：US1（菜单图标 + 5 禁用动作）
4. **STOP and VALIDATE**: `icon_action_map_test` 绿 + `--snapshot` 菜单图标快照 + 抽查 ≥10 项语义
5. 可交付 MVP：菜单图标化

### Incremental Delivery

1. Setup + Foundational → 基础设施就绪
2. US1 菜单图标 → 验证 → 交付（MVP）
3. US2 左侧功能栏 → 验证 → 交付（主功能主体：5 接线 + 5 禁用占位）
4. US3 右侧功能栏 → 验证 → 交付（9 按钮占位）
5. US4 应用图标（并行）→ 验证 → 交付
6. Polish：mockup 自查、缺失回退、全量回归、quickstart 验收

### Parallel Team Strategy

- 开发者 A：US1 → US2 → US3（MainWindow 文件顺序，契约单测护航）
- 开发者 B：US4 + Setup 图标（完全独立文件）
- 汇合点：Polish 阶段 mockup 自查与全量回归

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to user story（US1~US4）for traceability
- 契约单测（T010）必须先红后绿；实现不得在测试绿之前合并
- Commit after each task or logical group
- 停止点：US1 / US2 / US3 各自 Checkpoint 独立验证
- 共享文件 `src/ui/MainWindow.h/.cpp` 由 US1→US2→US3 顺序编辑，避免并发冲突
- 未实现功能按钮必须 `setEnabled(false)` 且不连接任何槽（宪法 IV）

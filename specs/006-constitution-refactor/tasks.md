# Tasks: Constitution Compliance Refactor

**Input**: Design documents from `/specs/006-constitution-refactor/`

**Prerequisites**: [plan.md](plan.md)（required）、[spec.md](spec.md)（required，user stories）、[research.md](research.md)、[data-model.md](data-model.md)、[contracts/interfaces.md](contracts/interfaces.md)、[quickstart.md](quickstart.md)

**Tests**: 本特性为行为等价重构，spec 未要求新增测试文件；回归由现有 `ctest`/`pytest` 保障（FR-011/FR-013）。每个 User Story 末尾的验证任务执行 [quickstart.md](quickstart.md) 对应场景。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1（P1 拆分巨型文件）、US2（P2 头文件合规）、US3（P3 类/函数/命名规范）
- Include exact file paths in descriptions

## Path Conventions

- **单项目**: `src/`、`tests/` at repository root；验证脚本位于 `scripts/`；本特性文档位于 `specs/006-constitution-refactor/`

---

## Phase 1: Setup（共享基础设施）

**Purpose**: 验证工具链与门禁脚本，为全库重构建立可自动验证的基础。

- [ ] T001 创建项目 `.clang-format`（Google 内置 style 导出为基线，针对 Qt 惯例微调：Qt 宏 `Q_OBJECT`/`signals`/`slots` 的排版、`AccessModifierOffset` 对齐 Qt 4 空格习惯前的最终确认），并全库 dry-run 试运行验证不破坏 moc 生成代码
- [ ] T002 [P] 新增 `scripts/check_line_counts.ps1`：递归统计 `src/` 与 `tests/cpp/` 下 `.cpp`/`.h`/`.hpp` 行数，对照上限（.cpp ≤800、.h 红线 500、.hpp ≤800）输出 ERROR 列表并返回非 0 退出码
- [ ] T003 [P] 新增 `scripts/check_pragma_once.ps1`：扫描 `src/` 与 `tests/cpp/` 全部 `.h`/`.hpp`，检测缺失 `#pragma once` 的文件并返回非 0 退出码
- [ ] T004 [P] 新增 `scripts/format_all.ps1`：调用 clang-format 全库改写；支持 `-Check` 参数（`--dry-run --Werror` 门禁模式），退出码区分"需改写/合规"
- [ ] T005 记录回归基线：在 `build/` 下执行 `ctest --test-dir build --output-on-failure` 与 `pytest tests/python -q`，确认 100% 通过并记录测试总数（作为行为等价对照基线）

---

## Phase 2: Foundational（阻塞性前置）

**Purpose**: 全库格式对齐（FR-014，spec 选项 B）。须在逻辑拆分前完成，保证拆分基于已格式化代码且 diff 可审查。

**⚠️ CRITICAL**: 未完成本阶段前不得开始任何 User Story。

- [ ] T006 执行全库格式对齐：运行 `scripts/format_all.ps1` 一次性改写 `src/` 与 `tests/cpp/` 全部源文件为 Google 格式；单独提交（commit message 标注 `style: apply google format`），确认 diff 中无任何逻辑改动
- [ ] T007 格式对齐后验证行为等价：重跑 `ctest` 与 `pytest`，确认与 T005 基线结果一致（100% 通过、测试断言行零变更）

---

## Phase 3: User Story 1（P1）- 拆分超标巨型源文件

**Goal**: `MainWindow.cpp`（1770 行）按职责拆分至 ≤800 行、`MainWindow.h` 成员函数 ≤30，行为不变。

**Independent Test**: `scripts/check_line_counts.ps1` 确认无 `.cpp` 超 800 行；`ctest`+`pytest` 全绿；`--snapshot` 截图与重构前一致（quickstart 场景 1/4/5）。

### 3.1 提取 WindowMaximizeController（多屏最大化，005 逻辑）

- [ ] T008 [US1] 创建 `src/ui/maximize/window_maximize_controller.h`：声明 `WindowMaximizeController`（契约见 contracts/interfaces.md §3），含 `handleWindowMessage(...)`、`toggleMaximize()`、`isMaximized()`、`maximizedChanged(bool)` 信号；成员 `name_` 后缀、全 private
- [ ] T009 [US1] [P] 实现 `src/ui/maximize/window_maximize_controller.cpp`：从 `src/ui/MainWindow.cpp` 迁移多屏最大化逻辑（`nativeEvent` 的 `WM_GETMINMAXINFO` 计算、`normalGeometry_` 记录/还原、最大化进出判定——保持 `ptMaxPosition` 以目标屏虚拟桌面坐标为准），类名大驼峰、成员 `name_` 后缀

### 3.2 提取 DockDragOverlay（Dock 拖拽高亮）

- [ ] T010 [US1] 创建 `src/ui/subwindow/dock_drag_overlay.h`：声明 `DockDragOverlay`（契约见 contracts/interfaces.md §3），含 `begin/update/end`、`sashBegin/sashUpdate/sashEnd`、`hitTest(QPoint)`、`highlightRect()`；成员 `name_` 后缀
- [ ] T011 [US1] [P] 实现 `src/ui/subwindow/dock_drag_overlay.cpp`：从 `src/ui/MainWindow.cpp` 迁移拖拽覆盖层逻辑（全窗口鼠标穿透覆盖层、`sashHitTest` 命中测试、`sashHighlightRect` 分割线高亮、放置坐标计算——保持 VSCode 风格高亮与局部重绘语义）

### 3.3 提取 LogSettingsController（日志设置）

- [ ] T012 [US1] 创建 `src/ui/log/log_settings_controller.h`：声明 `LogSettingsController`（契约见 contracts/interfaces.md §3），含 `restoreFromSettings()`、`setLogFilePath(QString)`、`applyLevel(LogLevel, bool)`、`applyAllLevels(bool)`、`openLogDir()`、`clearHistory()`
- [ ] T013 [US1] [P] 实现 `src/ui/log/log_settings_controller.cpp`：从 `src/ui/MainWindow.cpp` 迁移日志设置逻辑（级别勾选 `onLevelToggled`/`onAllLevels`、路径 `openLogDir`/`setLogPath`/`setLogFilePath`、`clearLogHistory`、`restoreLogSettings`），仅依赖 `core::log::Logger`/`LogLevelMatrix`，不引入 VTK/渲染依赖

### 3.4 MainWindow 瘦身与集成

- [ ] T014 [US1] 更新 `src/ui/MainWindow.h`：删除已迁移的成员函数声明与状态字段（`normalGeometry_`/`prevMaximized_`/日志相关字段/拖拽相关字段），新增三个控制器成员指针（`name_` 后缀、private）与转发方法声明；确认头文件成员函数 ≤30、<300 行
- [ ] T015 [US1] 更新 `src/ui/MainWindow.cpp`：删除已迁移实现，改为调用三个控制器；`nativeEvent`/`changeEvent`/拖拽入口/日志菜单连接保留为转发；`createActions`/`createMenus`/`createToolbars`/`createTitleBar` 等装配逻辑保留在 MainWindow；文件打开/导出（`openFile`/`addFileToTree`/`export*`）保留
- [ ] T016 [US1] 更新 `src/ui/CMakeLists.txt`：在 `add_library(perception_ui STATIC ...)` 注册 `maximize/window_maximize_controller.cpp`、`subwindow/dock_drag_overlay.cpp`、`log/log_settings_controller.cpp`
- [ ] T017 [US1] 验证 US1：运行 `scripts/check_line_counts.ps1`（`MainWindow.cpp` ≤800 且无新超标文件）；`ctest` + `pytest` 全绿；`--snapshot` 截图对比无差异；对照 contracts/interfaces.md §2 核对 `MainWindow`/`PythonConsole` 受保护接口签名未变

---

## Phase 4: User Story 2（P2）- 头文件合规化

**Goal**: 全部 `.h`/`.hpp` 带 `#pragma once`；普通 `.h` 仅含声明；`theme_catalog.h`（410 行）拆分至各 <300 行。

**Independent Test**: `scripts/check_pragma_once.ps1` 零缺失；`scripts/check_line_counts.ps1` 各 `theme_catalog*.h` <300；构建 + `ctest`/`pytest` 全绿（quickstart 场景 1/2/4）。

### 4.1 theme_catalog 按族拆分

- [ ] T018 [US2] 创建 `src/ui/theme/theme_catalog_dark.h`：迁移 15 个深色主题常量（`kDarkClassic`…`kAyuDark`，自 `src/ui/theme/theme_catalog.h`），`#pragma once` + `inline const ThemeColors`（C++17，无 ODR 问题），目标 <300 行
- [ ] T019 [US2] [P] 创建 `src/ui/theme/theme_catalog_light.h`：迁移 7 个浅色主题常量（`kLightClassic`…`kGithubLight`），`#pragma once`，目标 <100 行
- [ ] T020 [US2] [P] 创建 `src/ui/theme/theme_catalog_high_contrast.h`：迁移 3 个高对比主题常量（`kHighContrastBlack/White/Blue`），`#pragma once`，目标 <50 行
- [ ] T021 [US2] 更新 `src/ui/theme/theme_catalog.h` 为聚合入口：保留 `ThemeColors`/`ThemeDescriptor` 定义、`kThemes[]` 目录（**顺序不变**，菜单顺序）、`findTheme()`、`kThemeCount`/`kDefaultThemeId`/`kSettingsThemeKey`，`#include` 三个族头；目标 <300 行

### 4.2 头保护与声明/实现分离

- [ ] T022 [US2] 运行 `scripts/check_pragma_once.ps1` 扫描 `src/` 与 `tests/cpp/` 全部头文件，逐一修复缺失 `#pragma once` 的文件（如发现）
- [ ] T023 [US2] 审查普通 `.h` 中的业务实现：将非模板/inline 的实现迁移至对应 `.cpp`（模板/inline 逻辑按宪法例外保留）；以 `src/ui/theme/theme_catalog.h` 拆分后的聚合头为抽查基准，确认零普通 `.h` 含业务实现
- [ ] T024 [US2] 验证 US2：`scripts/check_pragma_once.ps1` 零缺失；`scripts/check_line_counts.ps1` 各 `theme_catalog*.h` <300；全量构建 + `ctest`/`pytest` 全绿；确认 `kThemes[]` 顺序与 `id` 值（QSettings 持久化键）未变

---

## Phase 5: User Story 3（P3）- 类/函数/命名规范对齐

**Goal**: 消除 God Class、超长函数、public 裸成员、不合规命名；`const`/`override` 齐全。

**Independent Test**: 静态扫描 + 代码评审对照宪法逐项核对零违规；`ctest`/`pytest` 全绿（quickstart 场景 6）。

- [ ] T025 [US3] 提取 `src/ui/console/PythonConsole.cpp` 的 Python C API 胶水（匿名命名空间 `ConsoleOut_*`/`cpp_log_impl`/`create_window_impl`/`kBootstrap`，约 215 行）至内部 TU `src/ui/console/python_bridge.cpp`（不新增公共头）；确认 MSVC Debug 链接（`python_debug_shim` 兼容层）不受影响；在 `src/ui/CMakeLists.txt` 注册
- [ ] T026 [US3] 命名规范扫描与修正：全量检索 `src/` 与 `tests/cpp/` 中的拼音命名、下划线开头变量、非大驼峰类名、无 `name_` 后缀成员变量（对齐 FR-010/宪法命名规范），逐处修正并保持行为不变
- [ ] T027 [US3] [P] public 裸成员审查：对照 FR-007 检查全部类的成员变量可见性，public 裸暴露者封装为 get/set 并更新调用点
- [ ] T028 [US3] [P] `const`/`override` 补全：对照 FR-008 审查全部成员函数（不修改状态者补 `const`，重写虚函数补 `override`）；基类多态析构确认 `virtual`
- [ ] T029 [US3] 超长函数拆分：审查 `src/ui/MainWindow.cpp` 拆分后与全库剩余文件中的 >120 行函数（已知接近阈值：`createActions`/`createMenus`/`createDocks`），按单一职责拆分
- [ ] T030 [US3] 验证 US3：代码评审对照宪法「类编码约束」「函数约束」「命名规范」零违规项；`ctest` + `pytest` 全绿；确认无新增第三方运行依赖（`git diff` 核对 `CMakeLists.txt`/依赖清单）

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 文档同步、最终门禁与整体验收。

- [ ] T031 更新 `docs/architecture/flows.md`：补充/调整模块图（MainWindow 拆分出的 `WindowMaximizeController`/`DockDragOverlay`/`LogSettingsController` 与 theme_catalog 族头结构），与 `src/ui/` 实际目录一致
- [ ] T032 核对 `src/ui/CMakeLists.txt` 与全部新增/迁移源文件一致（含 `python_bridge.cpp`），全量构建零警告级错误
- [ ] T033 最终门禁：依次运行 `scripts/check_line_counts.ps1`、`scripts/check_pragma_once.ps1`、`scripts/format_all.ps1 -Check`、`ctest`、`pytest`，全部通过；对照 spec.md Success Criteria SC-001~SC-007 逐项确认

---

## Dependencies

```text
Phase 1 (T001-T005)
        │
        ▼
Phase 2 (T006-T007)   ← 阻塞性：所有 User Story 依赖全库格式对齐
        │
        ▼
Phase 3: US1 (T008-T017)   P1 拆分 MainWindow
        │
        ▼
Phase 4: US2 (T018-T024)   P2 头文件合规
        │
        ▼
Phase 5: US3 (T025-T030)   P3 类/函数/命名规范
        │
        ▼
Phase 6 (T031-T033)   Polish & 门禁
```

### Within Each User Story

- 头文件（声明）先于实现文件（.cpp）
- 三个控制器（3.1/3.2/3.3）相互独立，任一线程内 `h`→`cpp` 顺序执行
- MainWindow 瘦身（T014/T015）须等三个控制器实现完成
- Story 完成后验证（T017/T024/T030）通过才进入下一优先级

### Parallel Opportunities

- Setup：T002/T003/T004 可并行（三个独立脚本）
- US1：T008/T010/T012（三个新头文件）并行；T009/T011/T013（三个实现，不同文件）并行
- US2：T018/T019/T020（三个族头）并行
- US3：T027/T028（public 成员审查 与 const/override 审查，不同关注面）并行
- 同一任务串内（h→cpp）不可并行；MainWindow 相关任务（T014/T015）不可与控制器提取并行

---

## Parallel Example: User Story 1

```bash
# 三个新头文件并行：
Task: "T008 Create src/ui/maximize/window_maximize_controller.h"
Task: "T010 Create src/ui/subwindow/dock_drag_overlay.h"
Task: "T012 Create src/ui/log/log_settings_controller.h"

# 三个实现文件并行：
Task: "T009 Implement src/ui/maximize/window_maximize_controller.cpp"
Task: "T011 Implement src/ui/subwindow/dock_drag_overlay.cpp"
Task: "T013 Implement src/ui/log/log_settings_controller.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. 完成 Phase 1：Setup（脚本与门禁 + 基线记录）
2. 完成 Phase 2：Foundational（全库格式对齐，单独提交）——**CRITICAL，阻塞所有故事**
3. 完成 Phase 3：US1（MainWindow 拆分）
4. **STOP and VALIDATE**: 独立验证 US1（行数 ≤800 + 测试全绿 + 截图对比）
5. US1 即 MVP 交付（最大违规项消除）

### Incremental Delivery

1. Setup + Foundational → 工具链与格式基线就绪
2. 完成 US1 → 独立验证 → 交付（MVP：`MainWindow.cpp` 1770 → ≤800 行）
3. 完成 US2 → 独立验证 → 交付（头文件合规）
4. 完成 US3 → 独立验证 → 交付（规范对齐）
5. 每阶段格式与逻辑分离提交，保持可回滚、可审查

### Parallel Team Strategy

1. 团队共同完成 Setup + Foundational
2. Foundational 完成后：
   - Developer A：US1（3 个控制器可拆分给 3 人并行）
   - Developer B：US2（theme_catalog 族头 + 头保护扫描）
   - Developer C：US3（命名/const/override 审查）
3. 各 Story 独立集成与验证

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- 每个 User Story 独立可完成、可测试
- 回归标准：`ctest` + `pytest` 100% 通过、测试断言行零变更（行为等价 FR-011）
- 格式对齐（T006）与逻辑变更分次提交；本特性文档位于 `specs/006-constitution-refactor/`
- 提交信息语义清晰（如 `style: apply google format`、`refactor: extract WindowMaximizeController`）
- 避免：模糊任务、同文件冲突、破坏 Story 独立性的跨 Story 依赖
- 发现的既有 bug 不在此特性修复，单独走 spec 流程（spec Edge Cases）

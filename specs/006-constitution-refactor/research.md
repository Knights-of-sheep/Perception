# Research: Constitution Compliance Refactor

**Branch**: `006-constitution-refactor` | **Date**: 2026-08-29 | **Plan**: [plan.md](plan.md)

## 决策总览

| 决策 | 选择 | 状态 |
|------|------|------|
| D1 格式对齐工具 | clang-format（Google style + 项目 `.clang-format`） | 已定 |
| D2 MainWindow 拆分策略 | 按职责提取 3 个控制器类 + MainWindow 瘦身 | 已定 |
| D3 theme_catalog 拆分 | 按主题族拆为 4 个数据头 | 已定 |
| D4 PythonConsole 调整 | 提取 Python C API 胶水到内部 TU（P3） | 已定 |
| D5 行为等价保障 | 先建立回归基线，小步提交 | 已定 |

---

## D1: 格式对齐工具

**Decision**: 引入 `clang-format`（LLVM 工具）作为开发工具，使用内置 `Google` style 为基础生成项目 `.clang-format`（缩进 2 空格、列宽 80、指针星号靠右等 Google 默认），必要时针对 Qt 惯例微调（如 `AccessModifierOffset`）。新增 `scripts/format_all.ps1`（全库改写）与 `scripts/check_format.ps1`（`clang-format --dry-run --Werror` 门禁）。

**Rationale**:
- Google C++ Style Guide 是宪法 FR-014 的格式标准；clang-format 是官方推荐的自动化格式化工具，其内置 `Google` 风格与该规范一一对应。
- 工具化而非人工对齐保证可复现、可门禁（SC-007 可自动验证）。
- 它是开发工具而非运行依赖/构建系统，不违反 FR-012 与「技术栈约束」。
- 项目为 CMake 工程，clang-format 不介入构建（仅 dev 脚本调用）。

**Alternatives considered**:
- 人工逐文件对齐：不可复现、评审负担重、易遗漏 → 拒绝。
- 引入 Qt Creator 格式化：非标准、无 CLI 门禁能力 → 拒绝。
- 仅在 CI 中检查不改写：需要先有历史全库基线 → 采用脚本一次改写 + 之后门禁。

**落地**：`clang-format` 需在开发者环境可用（`winget install LLVM` 或 VS2022 组件）；`format_all.ps1` 支持 `-Check` 参数以复用为门禁脚本。格式化与逻辑变更分次提交（spec Assumptions）。

---

## D2: MainWindow 拆分策略

**Decision**: 按职责聚类将 `MainWindow`（cpp 1770 行、头文件成员函数 50+）拆分为：

1. **`WindowMaximizeController`**（`src/ui/maximize/window_maximize_controller.{h,cpp}`）：承接 005 多屏最大化逻辑——`nativeEvent` 中 `WM_GETMINMAXINFO` 的 `MINMAXINFO` 计算（目标屏坐标）、`normalGeometry_` 记录/还原、最大化进出判定。MainWindow 保留 `nativeEvent`/`changeEvent` 虚函数入口并转发。
2. **`DockDragOverlay`**（`src/ui/subwindow/dock_drag_overlay.{h,cpp}`）：承接 Dock 拖拽高亮覆盖层——全窗口鼠标穿透覆盖层 widget、命中测试（`sashHitTest`）、分割线高亮绘制（`sashHighlightRect`）、放置逻辑。MainWindow 保留 `beginDockDrag`/`updateDockDrag`/`endDockDrag` 编排入口。
3. **`LogSettingsController`**（`src/ui/log/log_settings_controller.{h,cpp}`）：承接日志设置 UI——级别勾选（`onLevelToggled`/`onAllLevels`）、日志路径（`openLogDir`/`setLogPath`/`setLogFilePath`）、历史清除（`clearLogHistory`）、设置恢复（`restoreLogSettings`）。依赖 `core::log::Logger`/`LogLevelMatrix`。
4. **MainWindow 瘦身**：保留动作/菜单/工具栏装配（`createActions`/`createMenus`/`createToolbars`/`createTitleBar`）、事件入口、子窗口/Python 控制台编排、主题应用（`applyTheme`）与文件/导出（`openFile`/`addFileToTree`/`export*`）。

**Rationale**:
- 三个候选控制器职责边界清晰（日志 / 多屏窗口 / Dock 拖拽），与 flows.md 的 6.1/6.2/6.3 功能分区一一对应，便于独立测试。
- 提取后 MainWindow 成员函数可由 50+ 降至 ~30（达到 FR-006），cpp 行数目标 ≤800（FR-001）。
- 拆分单元间通过 MainWindow 持有的成员引用协作，无循环依赖（控制器单向引用 MainWindow 状态或经 setter 注入）。

**Alternatives considered**:
- 仅机械按行区间切分（如 0-600/600-1200/1200-1800）：切分点无职责语义、引入隐式共享状态 → 拒绝。
- 将所有 UI 装配（菜单/工具栏）也外提：改动面过大、风险高，超出本特性必要范围（保持行为等价原则）→ 延后。
- 将 Dock 拖拽并入既有 `LayoutManager`：`LayoutManager` 负责子窗口网格布局，Dock 覆盖层属于 QMainWindow 级交互，职责不同 → 拒绝。

**风险与缓解**：`nativeEvent` 为 Windows 专属消息处理，提取逻辑须保留事件顺序语义；以 `window_geometry_test`、多屏冒烟测试为护栏。

---

## D3: theme_catalog 拆分

**Decision**: `theme_catalog.h`（410 行）拆为聚合入口 + 3 个族数据头：

- `theme_catalog.h`：`ThemeColors`/`ThemeDescriptor` 定义、`kThemes[]` 目录、`findTheme()`、`kThemeCount`/`kDefaultThemeId`/`kSettingsThemeKey`，并 `#include` 三个族头（~60 行）。
- `theme_catalog_dark.h`：15 个深色主题常量（~220 行）。
- `theme_catalog_light.h`：7 个浅色主题常量（~100 行）。
- `theme_catalog_high_contrast.h`：3 个高对比主题常量（~45 行）。

**Rationale**:
- 所有色板为 `inline const` 变量（C++17），跨头文件/跨 TU 无 ODR 问题。
- 每个文件 <300 行建议值（FR-002）；`kThemes` 聚合关系保持不变，`ThemeManager` 消费方零改动（接口契约不变）。
- 数据仍为"唯一语义色定义"，单一职责不变。

**Alternatives considered**:
- 保持 410 行不动并豁免：> 建议值 300，SC-001 的"≥90% 达成率"难以证明 → 拒绝。
- 拆为 `.cpp` 定义 + getter：改变数据可见性/访问方式，消费方改动大 → 拒绝。
- 按主题逐个拆分：过度碎片化 → 拒绝。

---

## D4: PythonConsole 调整

**Decision**: `PythonConsole.cpp`（704 行）在 P3 阶段将匿名命名空间内的 Python C API 胶水（`ConsoleOut_*` 方法表、`cpp_log_impl`、`create_window_impl`、`kBootstrap`，约 215 行）提取至内部 TU `src/ui/console/python_bridge.cpp`（不生成头文件；`PythonConsole.cpp` 内通过前置声明/extern 调用，或提取一组窄接口）。提取后 `PythonConsole.cpp` 约 500 行，成员函数 27 个保持。

**Rationale**:
- 胶水职责（PyMethodDef 表、GIL/输出重定向、引导脚本）与 REPL 交互逻辑（输入/历史/补全/执行）是两类不同职责（FR-001 单一职责、FR-006）。
- `python_debug_shim.cpp` 已有"内部 TU 胶水"先例，模式一致；Python.h 依赖本就隔离在 console/ 目录。
- 704 行本合规（<800），故列为 P3 可选；若实施需同步确认 MSVC Debug 链接不受影响。

**Alternatives considered**:
- 保持现状：合规但不满足职责单一优化目标 → 作为 P3 备选。
- 拆为公共头 + 多个 cpp：暴露过多内部符号 → 拒绝。

---

## D5: 行为等价保障

**Decision**: 采用"基线先行 + 小步提交"策略：任何逻辑性改动前先跑 `ctest` + `pytest` 建立绿基线；每次拆分提交后立即重跑；全库格式对齐单独提交（与逻辑变更分离）。新增 `scripts/check_line_counts.ps1` 与 `scripts/check_pragma_once.ps1` 作为自动门禁。

**Rationale**: FR-011 要求行为等价；小步提交使回归可定位、可回滚。格式与逻辑分次提交是 spec Assumptions 明确要求（选项 B 的配套约定）。

**Alternatives considered**:
- 一次性大 PR：回归难以定位 → 拒绝。

---

## 未决项

- `clang-format` 对 Qt 宏（`Q_OBJECT`、`signals`/`slots`）与 moc 生成代码的处理：计划阶段在 `.clang-format` 中针对 Qt 惯例微调并全库试运行确认无破坏；此为实施细节，不阻塞设计。
- `tests/cpp/logger_test.cpp`（316 行）与 `src/app/main.cpp`（309 行）超过 `.h` 建议值 300 行——按 spec Assumptions「测试文件建议值从宽」与"`.h` 建议值适用对象为头文件"处理：`main.cpp` 属 `.cpp`（上限 800，合规），`logger_test.cpp` 属测试（建议值从宽），均不强制拆分。

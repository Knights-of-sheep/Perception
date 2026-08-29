# Interface Contracts: Constitution Compliance Refactor

**Branch**: `006-constitution-refactor` | **Date**: 2026-08-29 | **Plan**: [plan.md](../plan.md)

> 本特性为行为等价重构。以下接口为本项目的**受保护契约**：重构后签名、语义与事件流必须保持不变。违反即视为回归，PR 拒绝合并。

## 1. 核心层接口（`src/core/`，本特性零改动）

| 接口 | 位置 | 契约内容 |
|------|------|----------|
| `IReader` | `src/core/io/reader.h` | `virtual std::string formatName() const`；`virtual ~IReader() = default` |
| `ICurveReader` | `src/core/io/reader.h` | `readCurves(...)` → `std::shared_ptr<model::IDataSet>`；失败抛含路径与原因的 `std::runtime_error` |
| `EventBus` | `src/core/event/event_bus.h` | `subscribe/unsubscribe` + `Token`；`ScopedSubscription` RAII 防悬挂回调；数据变更先 `publish` 再返回 |
| `model::IDataSet` / `Curve` / `Structure` | `src/core/model/` | 格式无关数据模型，字段与访问方法不变 |
| `core::log::Logger` / `LogLevelMatrix` / sinks | `src/core/log/` | 单例入口、级别矩阵、FileSink/TerminalSink 行为不变 |

## 2. UI 层受保护接口

| 接口 | 位置 | 契约内容 |
|------|------|----------|
| `MainWindow` 公共/槽接口 | `src/ui/MainWindow.h` | `pythonConsole()`、`centralContainer()`、`createSubwindow(QString)`、`beginDockDrag/updateDockDrag/endDockDrag`、`applyTheme(QString)`、`resetLayout()`、`openLayoutSettings()`、`showHiddenSubwindows()`、`shutdownPython()` —— 签名与语义不变；`main.cpp` 与 `PythonConsole` 回调依赖 |
| `PythonConsole` 命令入口 | `src/ui/console/PythonConsole.h` | `executeCommand(...)`、`setCreateWindowCallback(...)`、`requestCreateWindow(...)` —— REPL 命令执行路径不变 |
| 主题契约 | `src/ui/theme/` | `ThemeManager` 公开方法不变；`theme_catalog` 的 `kThemes` 顺序、`findTheme(id)` 兜底、`id` 值（QSettings 持久化键）不变 |
| 无边框弹窗 | `src/ui/dialog_title_bar.h` | 弹窗标题栏契约不变（宪法 GUI 条款） |

## 3. 新增接口（重构产出，进入契约）

| 接口 | 位置 | 契约内容 |
|------|------|----------|
| `WindowMaximizeController` | `src/ui/maximize/window_maximize_controller.h` | `handleWindowMessage(...)`、`toggleMaximize()`、`isMaximized()`、`maximizedChanged(bool)` 信号 |
| `DockDragOverlay` | `src/ui/subwindow/dock_drag_overlay.h` | `begin/update/end`、`sashBegin/sashUpdate/sashEnd`、`hitTest(QPoint)`、`highlightRect()` |
| `LogSettingsController` | `src/ui/log/log_settings_controller.h` | `restoreFromSettings()`、`setLogFilePath(QString)`、`applyLevel/applyAllLevels`、`openLogDir()`、`clearHistory()` |
| `ThemedFileDialog` | `src/ui/themed_file_dialog.h` | `runThemedFileDialog(parent, title, dir, filter, mode)`（Open/Save/Directory）——无边框文件对话框统一入口，MainWindow 与 LogSettingsController 共用 |
| `DockTitleBar` | `src/ui/subwindow/dock_title_bar.h` | 自定义 Dock 标题栏（停靠/浮动按钮集 + 拖拽高亮拦截），自 MainWindow.cpp 提取，行为不变 |
| `FramelessDialog` | `src/ui/frameless_dialog.h` | `showFramelessDialog(parent, title, html)`——帮助/关于无边框弹窗统一入口，行为不变 |
| `MainWindow_assembly.cpp` | `src/ui/MainWindow_assembly.cpp` | `MainWindow` 装配函数（createActions/createMenus/createTitleBar/createToolbars/setActionIcon/refreshActionIcons/updateWindowButtonIcons）的跨文件定义；动作/菜单/图标装配行为不变 |
| `python_bridge` | `src/ui/console/python_bridge.h/.cpp` | PythonConsole 的 CPython 胶水层（内部 TU）：`injectBridgeFunctions`（_cpp_log / create_window）、`installConsoleRedirects`（ConsoleOut 重定向）、`kBootstrap` 引导脚本；行为不变 |
| `theme_types.h` | `src/ui/theme/theme_types.h` | ThemeColors / ThemeDescriptor 结构唯一定义（theme_catalog 主头与色板分组头共用，避免循环依赖） |
| 色板分组头 | `src/ui/theme/theme_catalog_{dark,light,hc}.h` | 25 套主题色板按 深色(15)/浅色(10)/高对比(3) 分组；`theme_catalog.h` 保留目录与 `findTheme` 查询（公共 API 不变） |

## 4. 事件流不变式

## 4. 事件流不变式

- 数据变更必须先 `EventBus.publish` 再返回；回调中不重复发布同类型事件。
- 多屏最大化：`ptMaxPosition` 以目标屏虚拟桌面坐标为准（005 修复点），重构不得回退。
- 日志装配顺序（main.cpp）：ThemeManager → MainWindow → Logger::configure → restoreLogSettings → installQtMessageBridge，重构不得改变。

## 5. 验证方法

- 编译验证：全量构建（`scripts/build.ps1`）零错误。
- 签名核对：对照本契约人工评审；核心接口文件 `git diff` 应为空（除格式对齐）。
- 行为验证：`ctest` + `pytest` 全绿；`--snapshot` 截图与重构前对比。

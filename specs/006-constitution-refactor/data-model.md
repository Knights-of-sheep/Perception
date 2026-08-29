# Data Model: Constitution Compliance Refactor

**Branch**: `006-constitution-refactor` | **Date**: 2026-08-29 | **Plan**: [plan.md](plan.md)

> 本特性的"数据模型"指**重构后的目标类结构与文件归属**。业务数据模型（core 的 model/io/process/event）不受本次重构影响，仅作为受保护接口列入 [contracts/interfaces.md](contracts/interfaces.md)。

## 目标结构实体

### 1. MainWindow（瘦身后）

- **归属**: `src/ui/MainWindow.h/.cpp`
- **职责**: 主窗口协调者——动作/菜单/工具栏装配、事件入口转发、子窗口与 Python 控制台编排、主题应用、文件打开/导出、布局设置入口。
- **约束**: `.cpp` ≤ 800 行；头文件成员函数 ≤ 30（FR-006）；成员变量全部 private + get/set（FR-007）。
- **状态字段（保留）**: `normalGeometry_`、`prevMaximized_`、动作指针组、三个 Dock、`SubwindowContainer*`、`PythonConsole*`、`LogSettingsController*`、`WindowMaximizeController*`、`DockDragOverlay*`。
- **转发关系**（MainWindow 持有的控制器，单向依赖，无循环）:
  - `WindowMaximizeController*` ← 事件入口转发
  - `DockDragOverlay*` ← 拖拽编排
  - `LogSettingsController*` ← 菜单信号转发

### 2. WindowMaximizeController

- **归属**: `src/ui/maximize/window_maximize_controller.h/.cpp`（新增）
- **职责**: 多屏最大化（005 特性）——`WM_GETMINMAXINFO` 计算（`ptMaxPosition` 相对目标屏坐标）、`normalGeometry_` 记录与还原、最大化进出判定、窗口按钮图标状态同步信号。
- **输入**: 持有目标 `QMainWindow*`（或注入最小接口）；接收 `nativeEvent` 解析出的窗口消息。
- **输出**: 几何计算/还原动作；`maximizedChanged(bool)` 信号。
- **状态字段**: `normalGeometry_`、`prevMaximized_`、目标屏索引。
- **接口**（契约，见 contracts）: `handleWindowMessage(MSG*)`、`toggleMaximize()`、`isMaximized()`。
- **约束**: `.h` < 300 行；成员函数 ≤ 30；类名大驼峰、成员 `name_` 后缀。

### 3. DockDragOverlay

- **归属**: `src/ui/subwindow/dock_drag_overlay.h/.cpp`（新增）
- **职责**: Dock 拖拽放置覆盖层（flows.md §6.3）——全窗口鼠标穿透覆盖层显示/隐藏、`sashHitTest` 命中测试、`sashHighlightRect` 分割线高亮、放置坐标计算。
- **输入**: 目标 `QMainWindow*`（或覆盖层父 widget）；拖拽起点/鼠标位置。
- **输出**: 高亮矩形区域；放置区域判定结果。
- **状态字段**: 覆盖层 widget 指针、当前高亮矩形、拖拽类型（dock/sash）。
- **接口**（契约）: `begin(QWidget* dockTitleBar, QPoint)`、`update(QPoint)`、`end(QPoint)`、`sashBegin(int index)`、`sashUpdate()`、`sashEnd()`、`hitTest(QPoint)`。
- **约束**: `.h` < 300 行；成员函数 ≤ 30。

### 4. LogSettingsController

- **归属**: `src/ui/log/log_settings_controller.h/.cpp`（新增）
- **职责**: 日志设置菜单行为（flows.md §4）——级别勾选（全局矩阵）、VTK 日志开关、日志路径设置、历史清除、`QSettings` 恢复。
- **输入**: 菜单动作组引用；`core::log::Logger`/`LogLevelMatrix` 引用。
- **输出**: 配置变更经 Logger 生效；无数据层直接写入。
- **状态字段**: 动作指针组、日志路径、级别矩阵状态。
- **接口**（契约）: `restoreFromSettings()`、`setLogFilePath(QString)`、`applyLevel(LogLevel, bool)`、`applyAllLevels(bool)`、`openLogDir()`、`clearHistory()`。
- **约束**: `.h` < 300 行；成员函数 ≤ 30；不依赖 VTK/渲染层。

### 5. PythonConsole（调整）

- **归属**: `src/ui/console/PythonConsole.h/.cpp`（P3 可选：`+ python_bridge.cpp`）
- **职责**: 内嵌 CPython REPL（输入/历史/补全/执行/输出重定向）；命令执行路径与 `create_window()` 回调保持不变。
- **约束**: `.cpp` ≤ 800 行（当前 704 合规）；成员函数 ≤ 30（当前 27）。
- **注**: P3 提取胶水后 `python_bridge.cpp` 为内部 TU，不新增公共头。

### 6. theme_catalog 族头文件

- **归属**: `src/ui/theme/theme_catalog.h`（聚合入口）+ `theme_catalog_dark.h` / `theme_catalog_light.h` / `theme_catalog_high_contrast.h`（数据）
- **数据字段**: `ThemeColors`（36 个语义色 token）、`ThemeDescriptor`（id/name/family/colors）、`kThemes[25]` 目录、`findTheme(id)`。
- **拆分映射**:
  - 深色 15（kDarkClassic … kAyuDark）→ `theme_catalog_dark.h`
  - 浅色 7（kLightClassic … kGithubLight）→ `theme_catalog_light.h`
  - 高对比 3（kHighContrastBlack/White/Blue）→ `theme_catalog_high_contrast.h`
- **不变量**: `kThemes[]` 顺序（菜单顺序）不变；`id` 值不变（QSettings 持久化键，变更会破坏用户设置）；`findTheme` 兜底行为不变。
- **约束**: 每个族头 < 300 行；全部 `inline const`（C++17，无 ODR 问题）。

## 文件级约束映射

| 目标文件 | 当前行数 | 目标约束 | 达标方式 |
|----------|----------|----------|----------|
| `MainWindow.cpp` | 1770 | ≤ 800 | 提取 3 个控制器 |
| `MainWindow.h` | 232 | < 300 建议 / 成员函数 ≤ 30 | 成员函数随拆分外移 |
| `theme_catalog.h` | 410 | < 300 建议 | 拆为 4 头（聚合 + 3 族） |
| `PythonConsole.cpp` | 704 | ≤ 800（保持） | 提取胶水（P3 可选） |
| 其余 `src/` + `tests/cpp/` | 均 < 上限 | 分级上限 | 全库格式对齐 + 合规微调 |

## 状态转换

本重构不引入运行时状态机。唯一注意点：`WindowMaximizeController` 维护"普通/最大化/全屏"三态转换（继承自 005 既有逻辑），重构只平移不改变转换条件与顺序。

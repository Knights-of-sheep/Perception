# Research: 子窗口布局管理

> Phase 0 输出。输入：代码库探索报告 + 规格约束（spec FR-001~019 / SC-001~012）。

## 1. 布局容器技术选型

- **Decision**: 自研 `SubwindowContainer`（QWidget 派生）+ `LayoutManager` 纯逻辑类；排列基于 QGridLayout 动态重建。
- **Rationale**: 探索确认中央区域当前仅有占位 QLabel（`centralPlaceholder_`），全库无 QMdiArea/QSplitter/QTabWidget 视图容器代码。spec 要求无系统标题栏、一致间隙、最大化/全屏、相同宽高——自研容器对这些约束完全可控且轻量；QGridLayout 原生支持行列控制、统一 cell 尺寸（stretch）与 `setSpacing`。
- **Alternatives considered**:
  - QMdiArea：自带子窗口标题栏/装饰与窗口管理（级联/平铺），与"无 Qt 标题栏"直接冲突，QSS 定制受限，重量级 —— 放弃。
  - QSplitter：适合少量固定 split，多行列网格与统一宽高控制繁琐，间隙/单子窗口行为不灵活 —— 放弃。
  - QTabWidget：适合堆叠而非平铺排列，不符合三种排列模式 —— 放弃。

## 2. 无标题栏子窗口

- **Decision**: 子窗口为普通 QWidget 子控件（非顶层窗口，天然无系统标题栏），顶部内嵌自定义标题区（标题 + 最大化/全屏/关闭按钮），复用 `MainWindow.cpp` 内 `DockTitleBar` 的交互模式（实现时评估提取为公共组件）。
- **Rationale**: 子控件 QWidget 无窗口装饰，零成本满足 FR-018；标题区同时提供最大化/全屏触发入口与选中语义（spec Assumptions：单击任意区域选中）。
- **Alternatives considered**: 顶层窗口 + `Qt::FramelessWindowHint` 剥离装饰 —— 影响拖拽与层级管理，且需手工处理系统行为 —— 放弃。

## 3. 最大化 / 全屏实现与触发入口

- **Decision**:
  - 最大化（FR-016）：容器内仅显示选中子窗口（其余 hide），子窗口填满容器可用范围；退出恢复原排列。
  - 全屏（FR-017）：将选中子窗口 re-parent 至 MainWindow 顶层并覆盖整个工作区；面板显隐经 `QMainWindow::saveState/restoreState` 快照恢复（或逐 dock toggle，实现时取其一）；退出 re-parent 回容器并恢复。
  - 触发入口（spec 委托 plan 层决定）：标题区按钮（最大化/全屏/关闭）+ 双击标题区（最大化）+ 标题区右键菜单 + `Esc` 退出全屏/最大化。
- **Rationale**: 最大化/全屏期间子窗口不销毁（hide/re-parent），渲染内容与视图状态天然保留（FR-019）；re-parent 保留 widget 内部状态。
- **Alternatives considered**:
  - 全屏用独立覆盖层 widget：需额外同步，重 parent 更简单 —— 放弃。
  - 提升为顶层窗口（`Qt::Window`）：带来系统标题栏或需 Frameless + 层级管理，与 FR-018 冲突 —— 放弃。

## 4. Python 命令桥接

- **Decision**: 复用 `PythonConsole` 既有 `_cpp_log` 桥接模式：`PyMethodDef` + `PyCFunction_New` 将 `create_window(name)` 注入 REPL 全局命名空间，回调经 `std::function` 转发至 MainWindow；同时为 `PythonConsole` 增加命令执行入口 `executeCommand`，供菜单"新建子窗口"动作构造并提交同一 `create_window` 命令行（命令文本回显、返回值打印，FR-002/FR-027/SC-018）。
- **Rationale**: 探索确认 pybind11 命令层（`perception_py`）未构建（`src/python` 仅占位 CMakeLists）；`PythonConsole` 已有经验证的 C API 注入先例（`_cpp_log`，`PythonConsole.cpp:115-134`），是最短路径。符合宪法 IV——创建子窗口（含菜单入口）统一经命令行执行，菜单不再产生绕过命令层的第三入口；布局管理本就不走命令层。
- **Alternatives considered**: 先构建 `perception_py`（pybind11）—— 属 M5 规划范围，且 spec 明确布局管理无 Python 接口 —— 放弃。

## 5. 布局逻辑可测性与测试策略

- **Decision**: `LayoutManager` 为纯 C++ 类（仅依赖 QtCore 的 QSize/QRect/枚举，不依赖 QWidget），`tests/cpp/layout_manager_test.cpp` 单测覆盖 FR-004~009、SC-003/004/009；UI 交互走 quickstart 手动验证 + 截图比对。
- **Rationale**: 满足宪法 II Test-First（红-绿-重构）；排列/约束/宽高/间隙为确定性计算，可穷举组合。
- **Alternatives considered**: 仅在 GUI 手动验证 —— 违反宪法 II 且无法覆盖 N=1..10 组合 —— 放弃。

## 6. 边界语义：最大行/列与容量不足

- **Decision**: 网格计算按 `cols = maxCols>0 ? min(n, maxCols) : n`，`rows = ceil(n/cols)`；当 `rows > maxRows`（容量不足）时保持列数 ≤ maxCols，行数继续增长，保证全部子窗口可见可访问（FR-014 / SC-002），容器提供滚动兜底。
- **Rationale**: spec Assumptions 明确最大行/列为**上限约束、非固定行列强制网格**、超出容量保持可访问；US3 场景 3（2×2 容量 + 第 5 个可访问）由此满足。
- **Alternatives considered**: 容量满后转 Tab 堆叠 / 滚动隐藏 —— 破坏"不遮挡、可访问"直白语义 —— 放弃。

## 7. 间隙与相同宽高实现

- **Decision**: 间隙用 `QGridLayout::setSpacing(4)`（SC-009：≥4px 且任意相邻间隙一致 <1px）；"保持相同宽高"用统一 cell（所有行列相同 stretch + 子窗口 sizePolicy 一致），容器 `resizeEvent` 统一重算。
- **Rationale**: spacing 天然一致；统一 stretch 保证随窗口等比缩放（FR-009 / SC-004）。单子窗口时无相邻间隙、铺满可用范围（Edge Cases：不因间隙产生多余留白）。
- **Alternatives considered**: 手工 `setGeometry` 计算 —— 易出偏差、resize 处理繁琐 —— 放弃。

## 8. 菜单与图标

- **Decision**: "视图"菜单新增"新建子窗口"与"布局…"动作（`createActions`/`createMenus`）；"新建子窗口"复用 `icon-map.yaml` 现有 `view-multi-view` 图标（已确认存在）；动作槽构造 `create_window("plot_N")` 命令文本并提交 `PythonConsole::executeCommand` 执行（命令回显、返回值打印，FR-002/FR-027）。
- **Rationale**: spec 指明入口为菜单栏且"视图"菜单语义匹配（现有 toggle 面板/重置布局动作同处）；图标零新增成本；经命令层执行保证与 PyShell 入口行为 100% 一致（SC-008）。
- **Alternatives considered**: 工具栏按钮 —— spec 未要求，属可选增强 —— 本次不做。

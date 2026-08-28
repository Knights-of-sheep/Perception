# Implementation Plan: 子窗口布局管理

**Branch**: `004-dock-layout-manager` | **Date**: 2026-08-28 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/004-dock-layout-manager/spec.md`

## Summary

在 Perception 主窗口中央区域引入多子窗口视图容器（subwindow container）：通过 PyShell Python 命令 `create_window(name)` 与主窗口"视图"菜单双入口创建**无系统标题栏**的渲染子窗口——菜单入口同样触发同一 `create_window` 命令行执行（命令文本在 PyShell 回显、返回值打印，不绕过命令层，FR-002/FR-027）；布局设置界面支持三种排列模式（按行/按列/网格）、最大行/最大列约束、"保持相同宽高"与一致的子窗口间隙；支持选中子窗口**最大化**（占满中央区域）与**全屏**（占满主窗口工作区，属性/数据/PyShell 面板临时隐藏）。

布局逻辑以可单测的纯 C++ 类 `LayoutManager` 实现（不依赖 QWidget），UI 层 `SubwindowContainer`/`SubwindowView` 负责交互与呈现；Python 桥接复用 `PythonConsole` 既有 `_cpp_log` 注入模式（C API `PyCFunction_New`），pybind11 命令层未构建故不引入。

## Technical Context

**Language/Version**: C++17（MSVC/VS2022，/utf-8 /MP12；core 加 /W4 /WX）；内嵌 CPython 3.13（`PythonConsole` 经 C API 直接嵌入，未用 pybind11）

**Primary Dependencies**: Qt 5.15.2 Widgets（AUTOMOC/AUTOUIC/AUTORCC）；VTK 9.4.1 已在顶层 find_package 但 `src/render` 未实装——本次**零 VTK 头引入**；pybind11 命令层（`perception_py`）未构建，不用于本次

**Storage**: N/A（纯 UI 功能，无持久化；v1 布局不持久化——spec Assumptions；现有 QSettings 的 dock 布局记忆不扩展）

**Testing**: CTest（`tests/cpp`；已有 UI 测试先例 `icon_action_map_test` 链接 `perception_ui`，新增 `layout_manager_test` 同模式）+ pytest（`tests/python` 当前全 skip，不新增）

**Target Platform**: Windows 10/11 x64 桌面（Qt 5.15.2 msvc2019_64）

**Project Type**: desktop-app（Qt Widgets 单窗口应用）

**Performance Goals**: 布局重排即时生效（用户无感知延迟）；创建子窗口 1 秒内出现（SC-007）；连续 50 次模式/属性切换无崩溃卡顿（SC-005）；20+ 子窗口排列无明显性能劣化（Edge Cases）

**Constraints**: Qt 5.15.2 Widgets + QSS + Fusion（深色主题优先）；C++17 禁用 C++20+ 语法；VTK 仅限 `src/render`（本次零触碰）；无外部 UI 框架、无 WebView；子窗口不显示系统标题栏（FR-018）；间隙一致且 ≥4px（SC-009）

**Scale/Scope**: 子窗口数量 0~20+；排列模式 3 种 × 最大行/列（1–10 整数）组合；每个子窗口含占位渲染视图（`src/render` 未实装，`ICurveChart` 尚不存在，FR-003 允许占位）

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/004-dock-layout-manager/spec.md` 已批准（FR-001~019，Clarifications 4 条闭环，checklist 16/16）
- [x] **Test-First**：布局计算为纯 C++ 类，Phase 1 给出 `tests/cpp/layout_manager_test.cpp` 单测方案；UI 交互经 quickstart 手动验证 + 截图比对
- [x] **Layered Core**：本次改动全部落在 `src/ui`（纯 UI 布局），不触碰 `src/core` 的 model/io/process/event
- [x] **Command-Driven**：创建子窗口统一经命令行执行——PyShell 命令与菜单栏入口均触发同一 `create_window` 命令行（菜单动作构造命令文本提交 `PythonConsole` 命令层执行，命令回显、返回值打印，FR-002/FR-027）；布局管理属宪法 IV 明示豁免的"纯 UI 变更（布局、主题、面板可见性、选中高亮）"，不走命令层
- [x] **Local Design Source**：`docs/design/mockups/` 无本功能视觉稿（spec Assumptions 确认，仅 `005-icon-set` 主窗口稿作风格参考）；沿用现有 QSS 主题与控件风格，无 figma 依赖
- [x] **Scope**：不涉及文件格式读取（io 层零改动），无格式范围问题（宪法 VI 不适用）
- [x] **Technology Stack**：Qt 5.15.2 Widgets / C++17 / CMake+Ninja 合规；不引入 VTK 头、不引入 pybind11（复用既有 `PythonConsole` 嵌入模式）

> 全部通过，无需 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/004-dock-layout-manager/
├── plan.md              # 本文档
├── research.md          # Phase 0：技术选型决策
├── data-model.md        # Phase 1：子窗口/布局配置/显示状态数据模型
├── quickstart.md        # Phase 1：端到端验证指南
├── contracts/           # Phase 1：Python 命令契约 + 布局接口契约
└── tasks.md             # Phase 2（/speckit.tasks 生成，不由此命令创建）
```

### Source Code (repository root)

```text
src/ui/
├── MainWindow.{h,cpp}               # 现有主窗口：接入 SubwindowContainer、View 菜单动作、命令桥、全屏面板协调
├── action_icon_map.{h,cpp}          # 现有：复用 view-multi-view 图标
├── console/PythonConsole.{h,cpp}    # 现有 REPL：新增 create_window 注入回调与命令执行入口 executeCommand（菜单复用触发命令行）
├── theme/...                        # 现有主题体系：QSS 增补子窗口/布局控件规则
└── subwindow/                       # 新增：子窗口与布局管理
    ├── subwindow_container.{h,cpp}     # 中央区域容器：持有子窗口、执行排列/最大化/退出
    ├── subwindow_view.{h,cpp}          # 单个渲染子窗口（无标题栏，自定义标题区 + 占位内容）
    ├── layout_manager.{h,cpp}          # 纯布局计算（模式/最大行列/相同宽高/间隙），可单测
    └── layout_settings_dialog.{h,cpp}  # 布局设置界面（模式/行/列/相同宽高）

tests/cpp/
├── ...                                # 现有测试
└── layout_manager_test.cpp            # 新增：布局计算单元测试（链接 perception_ui）

tests/python/
└── ...                                # 不新增（命令层未构建，保持全 skip）
```

**Structure Decision**: 布局逻辑与 UI 分离——`LayoutManager` 为纯 C++ 类（输入：子窗口数、模式、最大行列、可用尺寸、间隙 → 输出：行列数与各 cell 几何），仅依赖 QtCore 类型，使 Test-First 可行（`tests/cpp` 无窗口环境可测）。`SubwindowContainer` 承担 QWidget 容器与交互（排列执行、最大化/全屏、面板隐藏协调）。`SubwindowView` 复用 `MainWindow.cpp` 内 `DockTitleBar` 的无标题栏模式（实现时评估是否提取为公共组件）。全屏的中央区域重挂载与面板显隐恢复由 `MainWindow` 协调（需访问各 dock 成员）。

创建子窗口的命令行路径：菜单"视图 → 新建子窗口"动作槽构造 `create_window("plot_N")` 命令文本并提交 `PythonConsole::executeCommand`（与 PyShell 手工输入走同一执行路径），由 PythonConsole 统一回显命令文本、执行桥接回调并打印返回值（True/False）——满足 FR-002/FR-027/SC-018，避免绕过命令层产生第三入口。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

本功能全部检查通过，无违规，无需填写。

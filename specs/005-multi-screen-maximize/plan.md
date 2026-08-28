# Implementation Plan: Multi-Screen Maximize

**Branch**: `005-multi-screen-maximize` | **Date**: 2026-08-28 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/005-multi-screen-maximize/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

修复无边框主窗口的多屏最大化缺陷：将主界面拖到另一块屏幕后点击"最大化"，窗口落到错误屏幕/屏幕外导致"消失"。

根因（Phase 0 调研）：`MainWindow::nativeEvent` 的 `WM_GETMINMAXINFO` 分支使用 `QWidget::screen()` 解析"当前所在屏幕"。对于无边框（Frameless）顶层窗口，该解析在跨屏移动后不可靠（返回主屏或滞后屏幕），导致 `ptMaxPosition/ptMaxSize` 以错误屏幕工作区计算，最大化结果落到用户视线之外的屏幕。

方案：在 `WM_GETMINMAXINFO` 中改为基于**窗口实际几何中心**解析目标屏（`QGuiApplication::screenAt(frameGeometry().center())`，fallback 到 `screen()`/`primaryScreen`），以目标屏 `availableGeometry()` 计算最大化几何；同时在 `changeEvent`（WindowStateChange）中**显式保存/恢复最大化前 normal geometry**，规避 Qt 对 frameless 窗口恢复态几何不可靠的问题。屏幕解析辅助提取为独立头文件（`src/ui/window_geometry.h/.cpp`），逻辑可作为纯函数在 `tests/cpp` 单测。

## Technical Context

**Language/Version**: C++17（MSVC VS2022；禁止 C++20+ 语法）

**Primary Dependencies**: Qt 5.15.2 Widgets（Fusion + QSS）；Windows 窗口消息（WinUser.h `MINMAXINFO`，`WM_GETMINMAXINFO`）

**Storage**: N/A（纯会话内窗口状态，无持久化）

**Testing**: `tests/cpp` + CTest（`layout_manager_test` 同款模式）；GUI 单测在 `PERCEPTION_BUILD_GUI=ON` 下注册；屏幕解析/最大化几何计算作为纯函数单测；多屏端到端为手动验证（`quickstart.md`）

**Target Platform**: Windows 10/11 桌面（多屏扩展模式；主屏 + 副屏任意排列）

**Project Type**: desktop-app（Qt Widgets，无边框主窗口）

**Performance Goals**: N/A（即时 UI 操作；最大化路径为单次消息处理，无可度量性能目标）

**Constraints**: Qt 5.15.2 + C++17；保持现有无边框窗口设计（不引入系统标题栏）；不改变现有按钮/双击语义（`toggleMaximize` 入口不变）

**Scale/Scope**: 单主窗口 × ≥2 屏幕；覆盖分辨率变化、显示器连接/断开、混合 DPI 场景

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/005-multi-screen-maximize/spec.md` 已批准（宪法 I）
- [x] **Test-First**：先有测试用例（`tests/cpp`）；合并前 `ctest` + `pytest` 全绿（宪法 II）——屏幕解析/几何计算纯函数单测先行
- [x] **Layered Core**：改动落入 `src/core` 的 model/io/process/event 对应层，无跨层直接数据操作（宪法 III）——本功能为纯 UI 行为修复，不触碰数据层
- [x] **Command-Driven**：数据处理经命令层 / `perception`·`extract` Python 包，UI 未绕过（宪法 IV）——不涉及数据处理，无命令层需求
- [x] **Local Design Source**：UI 改动已对照 `docs/design/mockups/`，未引入 figma 依赖（宪法 V）——行为修复无新视觉设计，mockups 为图标集与本功能无关
- [x] **Scope**：涉及格式在 VTK / SVisual(.plt/.tdr) / HDF5(.h5/.hdf5) / 曲线(.csv/.dat) 生态内（宪法 VI）——不涉及文件格式
- [x] **Technology Stack**：依赖与版本符合技术栈约束（C++17 / Qt 5.15.2 / VTK 9.4.1 / CPython 3.13 / pybind11 / CMake+Ninja；VTK 仅限 `src/render/`）（宪法「技术栈约束」）——本功能仅用 Qt Widgets + Win32 消息，符合约束

> 任一检查不通过：先修订方案并注明处置，或如实填入下方 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/005-multi-screen-maximize/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── core/                # 数据层（本功能不涉及）
├── ui/
│   ├── MainWindow.cpp   # 修改：nativeEvent WM_GETMINMAXINFO 分支、changeEvent、toggleMaximize
│   ├── MainWindow.h     # 修改：新增成员（normal geometry 记录、目标屏解析调用）
│   └── window_geometry.h/.cpp  # 新增：屏幕解析 + 最大化几何计算的纯函数
└── app/

tests/
├── cpp/
│   ├── window_geometry_test.cpp  # 新增：目标屏解析/最大化几何/恢复几何单测
│   └── CMakeLists.txt            # 修改：注册新测试（PERCEPTION_BUILD_GUI=ON 下）
└── python/              # 不涉及
```

**Structure Decision**: 单项目结构（与现状一致）。屏幕解析与最大化几何计算提取为 `src/ui/window_geometry.{h,cpp}` 纯函数（不依赖 MainWindow 实例），便于 `tests/cpp` 直接单测（宪法 II Test-First）；`MainWindow` 仅接线（消息分发 + 状态记录），符合 004 已定"文件级拆分、易扩展"方向。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

无宪法违规，本节留空。本功能为单文件窗口行为修复，不引入新层、新包、新存储。

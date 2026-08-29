# Implementation Plan: Constitution Compliance Refactor

**Branch**: `006-constitution-refactor` | **Date**: 2026-08-29 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/006-constitution-refactor/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

按宪法 v4.0.0 对现有 C++ 代码库执行合规重构：将超标巨型源文件（`MainWindow.cpp` 1770 行 > 800）按单一职责拆分为多个控制器类；`theme_catalog.h`（410 行）按主题族拆分为 4 个数据头文件；`PythonConsole.cpp`（704 行）提取 Python C API 胶水（P3）；并对 `src/` 与 `tests/cpp/` 全库一次性执行 Google C++ Style Guide 格式对齐（用户已确认选项 B）。全程保持行为等价：以现有 `ctest`/`pytest` 为回归基线，不新增运行依赖，不改变任何公开接口签名。

## Technical Context

**Language/Version**: C++17（MSVC VS2022）；Qt 5.15.2 Widgets + QSS + Fusion；VTK 9.4.1（仅 `src/render/`）；CPython 3.13 + pybind11（仅 `src/ui/console/`）——均为宪法锁定的技术栈，重构不改变。

**Primary Dependencies**: 现有运行依赖不变（Qt / VTK / CPython）。**新增开发工具**：`clang-format`（Google 内置 style + 项目 `.clang-format` 微调）——仅作格式化检查与改写工具，非运行依赖、非构建系统，符合 FR-012。

**Storage**: QSettings（主题/布局/日志配置）与 `%APPDATA%/Perception/logs`——重构不涉及存储结构或键名。

**Testing**: CTest（`tests/cpp/`，现有 8 个测试文件）+ pytest（`tests/python/`）。新增验证手段：行数统计脚本、`#pragma once` 扫描脚本、`clang-format --dry-run --Werror` 格式门禁，均放入 `scripts/`。

**Target Platform**: Windows 10/11 桌面（VS2022 + Ninja 构建，`scripts/build.ps1`）。

**Project Type**: desktop-app（Qt Widgets，Dock 布局 + QSS 深色主题 + 内嵌 Python REPL）。

**Performance Goals**: 不适用——本特性为行为等价重构；约束为编译时间不显著劣化（新增少量编译单元可接受）。

**Constraints**: 行数上限（`.cpp` ≤ 800、普通 `.h` 建议 300/红线 500、`.hpp` ≤ 800）；Google C++ Style Guide 格式；行为等价（FR-011）；零新增运行依赖（FR-012）；测试全绿（FR-013）。

**Scale/Scope**: `src/` + `tests/cpp/` 全部自有 C++ 文件（约 47 个）。主改造：`MainWindow.{h,cpp}`、`theme_catalog.h`、`PythonConsole.cpp` + `src/ui/CMakeLists.txt` 注册 + 全库格式对齐。

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/006-constitution-refactor/spec.md` 已批准（2026-08-29，质量检查 14/14 通过）（宪法 I）
- [x] **Test-First**：以现有 `tests/cpp`（CTest）+ `tests/python`（pytest）为回归基线；合并前 `ctest` + `pytest` 全绿（宪法 II / 架构契约 Test-First）
- [x] **Layered Core**：改动保持在 `src/ui` 层内；不改变 `src/core` 的 model/io/process/event 分层与事件流（宪法 III / 架构契约 Layered Core）
- [x] **Command-Driven**：重构不改变命令层路径；`PythonConsole` 胶水提取不绕过命令执行（宪法 IV / 架构契约）
- [x] **Local Design Source**：无视觉/布局改动；全库格式对齐不改变界面呈现（宪法 V / 架构契约）
- [x] **Scope**：不涉及任何新文件格式（宪法 VI / 架构契约 Data-Visualization Scope）
- [x] **Technology Stack**：运行依赖与版本全部不变；`clang-format` 为开发工具，非运行依赖、非构建系统（宪法「技术栈约束」+「依赖约束」）

> 任一检查不通过：先修订方案并注明处置，或如实填入下方 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/006-constitution-refactor/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   └── interfaces.md    # 受保护接口契约（重构不得改变签名）
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── app/
│   └── main.cpp                    # 309 行（合规，仅格式对齐）
├── core/                           # 无结构改动，仅格式对齐 + 合规微调
│   ├── event/  ├── io/  ├── log/  ├── model/  └── process/
├── render/                         # M3b 未落地（空/占位）
├── python/                         # M5 未落地（仅 CMakeLists.txt）
└── ui/
    ├── MainWindow.h/.cpp           # 瘦身：拆分后 ≤800 行、成员函数 ≤30
    ├── action_icon_map.h/.cpp
    ├── console/
    │   ├── PythonConsole.h/.cpp    # 拆分后 ~500 行（P3 提取胶水后）
    │   └── python_bridge.cpp       # [新增] Python C API 胶水（P3）
    ├── dialog_title_bar.h/.cpp
    ├── log/
    │   ├── qt_message_bridge.h/.cpp
    │   └── log_settings_controller.h/.cpp   # [新增] 日志设置控制器
    ├── maximize/
    │   └── window_maximize_controller.h/.cpp # [新增] 多屏最大化逻辑
    ├── subwindow/
    │   ├── layout_manager.h/.cpp
    │   ├── layout_settings_dialog.h/.cpp
    │   ├── subwindow_container.h/.cpp
    │   ├── subwindow_view.h/.cpp
    │   └── dock_drag_overlay.h/.cpp          # [新增] Dock 拖拽高亮覆盖层
    ├── theme/
    │   ├── theme_catalog.h                   # 保留：struct + kThemes + findTheme（~60 行）
    │   ├── theme_catalog_dark.h              # [新增] 15 深色（~220 行）
    │   ├── theme_catalog_light.h             # [新增] 7 浅色（~100 行）
    │   ├── theme_catalog_high_contrast.h     # [新增] 3 高对比（~45 行）
    │   ├── theme_manager.h/.cpp
    │   └── icon_factory.h/.cpp
    ├── window_geometry.h/.cpp
    └── CMakeLists.txt               # 注册新增 .cpp 文件

tests/cpp/                         # 测试文件仅格式对齐；断言行零变更（FR-011）
scripts/
    ├── build.ps1 / clean.ps1      # 现有构建脚本（不变）
    ├── check_line_counts.ps1      # [新增] 行数上限检查（FR-001/002/003）
    ├── check_pragma_once.ps1      # [新增] #pragma once 扫描（FR-004）
    ├── format_all.ps1             # [新增] 全库 clang-format 对齐（FR-014）
    └── check_format.ps1           # [新增] clang-format --dry-run 门禁（SC-007）
```

**Structure Decision**: 单项目 `src/` 分层（core / render / ui / python）保持不变。UI 层内按职责新增子目录：`maximize/`（多屏最大化控制器，005 特性逻辑）、`subwindow/dock_drag_overlay.*`（Dock 拖拽高亮，与既有 layout 组件同目录）、`log/log_settings_controller.*`（日志设置，与 `log/qt_message_bridge` 同目录）、`console/python_bridge.cpp`（Python C API 胶水，仅内部 TU，不对外暴露头文件）。`theme_catalog` 数据按主题族拆分为 4 个头文件，`theme_catalog.h` 作为聚合入口（include 三个族头并保留 `kThemes`/`findTheme`）。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| （无）全库一次性格式对齐产生大 diff | spec 已确认（选项 B，2026-08-29） | 分步对齐会导致过渡期风格不一致、双标准并存 |

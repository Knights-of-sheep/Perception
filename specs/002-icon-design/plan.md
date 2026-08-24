# Implementation Plan: 功能按钮图标设计

**Branch**: `002-icon-design` | **Date**: 2026-08-25 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/002-icon-design/spec.md`

## Summary

为 Perception（对标 ParaView + SVisual 的数据可视化工具）交付一套完整的图标体系：**统一设计规范 + 全量功能按钮图标 + 应用程序主图标（品牌图标）+ 更新界面 mockups**。图标为原创自绘、线性描边风格、深色主题适配，覆盖「对标功能清单」中全部功能类别，五态（正常/悬停/按下/禁用/选中）清晰可辨。当前项目零图标资源（无 .ico/.svg/.png 资源文件、`.qrc` 仅含 QSS、`main.cpp` 未设窗口图标、Action 仅用 `QStyle::standardIcon` 占位），本功能从零建立图标资源与视觉规范，设计稿落至 `docs/design/mockups/`（宪法原则 II）。

## Technical Context

**Language/Version**: C++17 + Qt 5.15.2 Widgets（GUI 目标）；Python 3.8+（命令层，本功能不涉及）。本功能以设计交付为主，源代码仅涉及资源集成（`.qrc` + `main.cpp` 窗口图标）与 mockups。

**Primary Dependencies**: Qt5 Widgets（`QIcon` / `QStyle` / 资源系统）、CMake（`qt5_add_resources`）。VTK 9.4 不涉及。

**Storage**: N/A —— 图标交付为静态资源文件（`.svg` 源 + `.png` 位图 + `.ico`），经 `src/ui/theme/theme.qrc`（或新增 `icon.qrc`）打包进可执行文件。

**Testing**: 本功能为设计 + 资源交付，验收采用：① 规范符合性评审（对照 `contracts/conformance-checklist.md` 逐项检查）；② mockups 视觉评审；③ Qt 资源编译验证（`.qrc` 能正常编译链接、`QIcon` 可加载）；④ `ctest`/`pytest` 回归（确保无 core 破坏）。

**Target Platform**: Windows 10/11（MSVC，本项目构建目标）。

**Project Type**: desktop-app（科学数据可视化工具）。

**Performance Goals**: N/A —— 图标为静态资源，无运行时性能要求；SVG 源保证矢量可缩放（16px → 256px）。

**Constraints**: 深色主题固定调色板（`docs/design/ui-guidelines.md` §3.1 Token）；工具栏图标基准 16px；图标原创自绘、不依赖第三方图标库；UTF-8 文本；QSS/调色板单一事实源。

**Scale/Scope**: 功能图标约 60+ 枚（覆盖「对标功能清单」六大类别）、应用主图标 1 枚（多尺寸适配）、设计规范文档 1 份、mockups 更新 ≥1 份。

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| 宪法条款 | 判定 | 说明 |
|---|---|---|
| I. 规范先行 | ✅ 通过 | 本 spec（v1.x 含 3 条澄清）已批准，`checklists/requirements.md` 16/16 通过 |
| II. 本地设计源 | ✅ 通过 | 图标视觉稿输出至 `docs/design/mockups/005-icon-set/`（新增），无 Figma 依赖 |
| III. 分层核心 | ✅ 通过 | 纯 UI/设计交付，不触碰 `src/core` 四层 |
| IV. 命令驱动 + Python 包 | ✅ 通过 | 纯 UI 变更（宪法明确豁免：布局、主题、面板可见性等不涉及数据变更） |
| V. 测试先行 | ✅ 通过 | 无 core 代码变更；验收=规范符合性评审 + 资源编译验证 + 回归测试 |
| VI. ParaView + SVisual 定位 | ✅ 通过 | 图标对标 ParaView/SVisual 核心功能清单，覆盖 1D/2D/3D 相关功能按钮 |
| 设计约束（深色/调色板/Qt5/UTF-8） | ✅ 通过 | 图标色板引用 `ui-guidelines.md` Token，不新增色值 |
| 安全约束 | ✅ 通过 | 图标为静态资源，无文件输入路径 |
| 质量门禁 | ✅ 通过 | M2/M4 里程碑 UI 截图与 mockups 对比机制适用 |

**GATE 结论：全部通过，无违规，无需 Complexity Tracking 豁免。**

## Project Structure

### Documentation (this feature)

```text
specs/002-icon-design/
├── plan.md              # 本文件
├── research.md          # Phase 0 设计研究决策
├── data-model.md        # Phase 1 图标数据模型
├── quickstart.md        # Phase 1 验收验证指南
├── contracts/           # Phase 1 验收契约
│   ├── icon-style-spec.md       # 图标设计规范契约（风格/网格/尺寸/色板/状态，可检查）
│   ├── icon-function-map.md     # 图标-功能映射表 schema 与命名规则
│   └── conformance-checklist.md # 规范符合性检查清单（评审工具）
└── tasks.md             # Phase 2 输出（/speckit.tasks 命令生成，不在本命令范围）
```

### Source Code (repository root)

```text
docs/design/
└── mockups/
    └── 005-icon-set/            # 本功能视觉稿（图标集展示 + 应用图标）
        ├── preview.png
        └── notes.md

src/ui/
├── theme/
│   ├── theme.qrc                # 已有；新增图标资源条目或新建 icon.qrc
│   ├── icons/                   # 新增：图标资源目录
│   │   ├── app/app-icon.svg     # 应用主图标（矢量源）
│   │   ├── app/app-icon.ico     # 应用主图标（Windows 多尺寸）
│   │   ├── actions/             # 功能按钮图标（SVG 源 + PNG 位图）
│   │   │   ├── open.svg
│   │   │   ├── export.svg
│   │   │   └── ...
│   └── ...
├── actions/                     # ActionRegistry 落地后按映射表挂图标（后续需求）
└── mainwindow/ ...

src/app/
└── main.cpp                     # 新增：mainWindow.setWindowIcon(QIcon(":/perception/icons/app/app-icon.ico"))
```

**Structure Decision**: 图标资源集中放在 `src/ui/theme/icons/`（与主题同一资源前缀 `/perception/`），便于 `QSS`、`QAction`、窗口图标统一引用。设计源（SVG）与运行位图（PNG/ICO）同目录存放；`.qrc` 沿用既有 `theme.qrc` 或新增独立 `icon.qrc`（避免 QSS 模板与二进制资源耦合），由 `tasks.md` 确定最终取舍。mockups 新增 `005-icon-set/` 目录承载图标集视觉稿（宪法 II：视觉稿事实源）。

## Complexity Tracking

> **无** —— Constitution Check 全部通过，无需豁免记录。

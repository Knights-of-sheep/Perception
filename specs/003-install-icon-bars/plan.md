# Implementation Plan: 安装程序图标与功能图标

**Branch**: `003-install-icon-bars` | **Date**: 2026-08-25 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/003-install-icon-bars/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

将 002 交付的 52 枚功能图标与设计版应用图标「安装」到程序 UI：所有菜单项配置语义图标；主窗口新增左右两条纵向功能栏（左侧 10 个通用功能按钮：撤销/重做/加载文件/加载脚本/主界面截图/主界面视频录制/刷新/数据面板显隐/属性面板显隐/命令窗口显隐；右侧 9 个领域功能按钮：放大/缩小/自适应显示/重置视图 + 添加曲线/移除曲线/数据提取/坐标轴设置/图例）。图标缺口新增 4 枚 SVG（加载脚本、视频录制、刷新、面板显隐），遵循 002 图标设计规范（实心填充、16 网格、五态可派生）。未实现功能（撤销/重做/加载脚本/视频录制/刷新及右侧全部 9 枚）按用户确认以禁用态占位呈现，功能本体由后续独立 spec 实现。技术方案：Qt QToolBar 纵向布局 + QAction 统一动作/图标挂接 + 运行时派生五态图标 + 契约驱动的 C++ 单测。

## Technical Context

**Language/Version**: C++17 / Qt 5.15.2（Widgets）/ VTK 9.4（GUI 构建时）；Python 3（图标工具链）

**Primary Dependencies**: Qt5Widgets（QToolBar/QAction/QIcon）、Qt5Svg（图标渲染工具链）、`src/ui/theme/theme_template.qss`（深色主题 QSS，已含 QToolBar/QToolButton 基础样式）、`src/ui/theme/theme.qrc`（图标资源注册）、`src/ui/theme/icons/icon-map.yaml`（图标语义映射）、`scripts/render_icons.py` / `gen_qrc.py` / `check_icons.py`（图标管线，002 交付）

**Storage**: N/A（无数据存储；图标资源内嵌 qrc，启动即加载）

**Testing**: CTest（`tests/cpp/`，新增 `icon_action_map_test.cpp` 验证动作-图标映射契约）+ pytest（`tests/python/`，命令层回归）+ 手动 UI 验证（`--snapshot` 快照）+ `scripts/check_icons.py`（新图标符合性校验）

**Target Platform**: Windows 10/11（MSVC 多配置生成器）

**Project Type**: desktop-app（Qt5 Widgets + Dock 布局 + QSS 深色主题）

**Performance Goals**: 程序启动到功能栏图标全部就绪 < 1 秒（spec SC-002）；启动无图标缺失的视觉闪烁

**Constraints**: 深色主题固定调色板（ui-guidelines §3.1 Token）；Qt5 Widgets+Dock+QSS，不引入外部 UI 框架（宪法）；图标必须满足 icon-style-spec（002）风格/网格/色板/五态条款；纯 UI 变更不触碰 `src/core` 数据层；未实现功能不得绕过命令层（宪法 IV）

**Scale/Scope**: 主窗口 1 个（MainWindow）；菜单 5 组约 20 个操作项；左右功能栏 2 条共 19 个按钮；图标 52 + 4 枚新增

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| 宪法条款 | 检查结果 |
|---|---|
| I. 规范先行 | ✅ PASS — spec.md 已批准，本 plan 严格处于 specify→plan 流程内 |
| II. 本地设计源 | ⚠️ PASS（缓解）— 左右功能栏为增量布局，无独立 mockup；布局由用户明确指定（按钮清单），视觉语言完全复用已批准的 theme_template.qss QToolBar/QToolButton 样式与 002 图标集，不引入新独立界面、不使用任何外部设计源。缓解：tasks 阶段用 make_mockups.py 生成功能栏 mockup，实现后截图对比自查（宪法 II 末条） |
| III. 分层核心 | ✅ PASS — 纯 UI 变更（图标、菜单、工具栏），不触碰 model/io/process/event |
| IV. 命令驱动 | ✅ PASS — 未实现功能按钮保持禁用态，不绕过命令层触发任何数据处理；面板显隐/主题属「纯 UI 变更」豁免项 |
| V. 测试先行 | ✅ PASS — 新增 `icon_action_map_test.cpp`（先红后绿，断言动作-图标映射契约、按钮集合完整性）；ctest + pytest 全量回归 |
| VI. 数据可视化定位 | ✅ PASS — 无文件格式/读取器影响 |
| 设计约束（深色主题、Qt5 Widgets+Dock+QSS、UTF-8、前后端分离） | ✅ PASS — 全部沿用 |
| 安全约束 | ✅ PASS — 图标资源 qrc 内嵌、路径固定；新增 SVG 经 check_icons.py 校验；不新增任何外部文件输入 |
| 可扩展性 | ✅ PASS — 新图标沿用 002 扩展机制（actions/*.svg + icon-map.yaml 登记 + render/gen/check 管线），消费者不变 |

**Gate 结论**: **PASS**（1 项已缓解偏差：功能栏无独立 mockup，见上表 II）

## Project Structure

### Documentation (this feature)

```text
specs/003-install-icon-bars/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/
│   └── icon-action-map.md  # Phase 1 output：动作↔图标映射契约（可检查）
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/ui/
├── MainWindow.h / .cpp      # 新增 createToolbars()；动作集中创建并挂接图标（契约表驱动）；五态图标派生工具
├── theme/
│   ├── theme.qrc            # 由 scripts/gen_qrc.py 重新生成（含 4 枚新图标 PNG 条目）
│   ├── theme_template.qss   # 新增 #leftToolBar / #rightToolBar 纵向样式（在既有 QToolButton 样式基础上扩展）
│   └── icons/
│       ├── actions/         # 新增 4 枚 SVG：file-load-script / file-record-screen / view-refresh / view-panel-toggle
│       ├── png/actions/     # scripts/render_icons.py 生成（16/24/32）
│       └── icon-map.yaml    # 新增 4 条语义登记

scripts/
├── render_icons.py          # 不变（自动覆盖新 SVG）
├── gen_qrc.py               # 不变
└── check_icons.py           # 不变（校验新图标符合性）

tests/cpp/
└── icon_action_map_test.cpp # 新增：动作-图标映射契约测试（集合完整、图标资源存在、tooltip 非空、状态正确）
```

**Structure Decision**: 沿用现有单一工程结构。全部改动集中在 `src/ui/`（MainWindow + theme），图标管线与测试位置遵循既有约定（`scripts/`、`tests/cpp/`），无新增模块目录。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

无违规项，本表不适用（1 项偏差已在上方 Gate 缓解，无需复杂度豁免）。

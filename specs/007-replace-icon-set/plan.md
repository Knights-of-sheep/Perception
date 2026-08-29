# Implementation Plan: Replace Icon Set

**Branch**: `007-replace-icon-set` | **Date**: 2026-08-29 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/007-replace-icon-set/spec.md`

## Summary

将 `src/ui/theme/icons/actions/` 下 **58 枚功能图标**（保留 `app-icon` 程序图标与 `view-panel-console` Python 控制台显隐图标）全部替换为 **Google Material Icons 官方免费集（Figma 官方社区版）** 的图形：人工下载 SVG → 色值归一化（黑 → FG_TEXT）→ 入库 → 复用现有 `check_icons.py` 门禁与 `render_icons.py`/`gen_qrc.py`/`make_mockups.py`/`update_screenshots.ps1` 全链路重渲染 → 修订 `icon-style-spec.md` 至 v2.0.0（实心填充 → 线性圆角描边）→ 双评审验收（盲测 + 符合性清单）→ 文档同步（icon-spec / mockups / README）。

## Technical Context

**Language/Version**: 不涉及新代码。SVG 资产（Material Icons Apache-2.0）；沿用现有脚本链（Python 3.13 + PyQt5 + Pillow + PyYAML）。

**Primary Dependencies**: 无新增。复用 `scripts/` 下既有脚本；素材为本地 SVG 文件。

**Storage**: 文件系统资源——`src/ui/theme/icons/actions/*.svg`（58 枚替换）、`src/ui/theme/icons/png/actions/*.png`（渲染产物，重新生成）、`docs/design/mockups/005-icon-set/`（mockup 重生成）。

**Testing**: 自动化 = `scripts/check_icons.py`（P-01/N-01/覆盖/schema 门禁）+ 全量构建 + `ctest`/`pytest`（回归，无新增逻辑）；人工 = 语义盲测（≥10 枚 × 3 人，≥90%）+ 符合性清单勾选。

**Target Platform**: Windows 10/11（桌面应用，Qt 5.15.2 资源打包）。

**Project Type**: desktop-app（UI 主题资源替换）。

**Performance Goals**: 不适用（资源型变更）；渲染/构建时间无回归（现有脚本）。

**Constraints**: 色板 Token 白名单 P-01 不变；命名 N-01 不变；icon-map ↔ SVG 覆盖规则 1 继续成立；保留项（`app-icon`、`view-panel-console`）零改动；不引入 Figma 扩展/REST/MCP（宪法「本地设计源」）。

**Scale/Scope**: 58 枚 SVG 替换 + 58×3 PNG 重渲染 + 1 套契约修订（v1.1.0 → v2.0.0）+ 文档同步。

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/007-replace-icon-set/spec.md` 已批准（宪法 I）
- [x] **Test-First**：本功能为 UI 资源替换，无新增 C++/Python 业务逻辑；自动化验证 = `check_icons.py` 门禁 + 全量构建 + `ctest`/`pytest` 回归。**豁免说明**：无新增逻辑即无新增单测义务；图标可检查性由契约 + 门禁脚本保证（宪法「测试质量标准」适用于代码逻辑，本功能无代码改动）
- [x] **Layered Core**：不涉及 `src/core`（宪法 III）
- [x] **Command-Driven**：不涉及数据处理（宪法 IV）
- [x] **Local Design Source**：素材人工下载**本地化**为仓库资产，未引入 figma 扩展 / Figma REST / MCP（宪法 V）
- [x] **Scope**：不涉及文件格式（宪法 VI）
- [x] **Technology Stack**：无新增依赖；复用 C++17 / Qt 5.15.2 / CPython 3.13 既有技术栈（宪法「技术栈约束」）

> 全部通过，无需 Complexity Tracking 豁免。

## Project Structure

### Documentation (this feature)

```text
specs/007-replace-icon-set/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output（6 项决策，未知项全部解决）
├── data-model.md        # Phase 1 output（Icon / IconMap 数据模型）
├── quickstart.md        # Phase 1 output（端到端验证指南）
├── contracts/           # Phase 1 output
│   ├── icon-replacement-map.md   # 58 枚 icon_id → Material 图标名映射（核心契约）
│   └── icon-source-and-style.md  # 素材来源规范 + 风格契约 v2.0.0 修订要点
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/ui/theme/icons/           # 本功能唯一代码改动面（资源）
├── actions/                  # 58 枚 SVG 替换（view-panel-console 保留）
├── app/                      # app-icon.svg / .ico —— 保留，零改动
├── png/                      # render_icons.py 重新生成（actions 58×3 + app 7 + ico）
├── templates/                # 不涉及
└── icon-map.yaml             # 不涉及（59 条目不变，仅图形替换）

docs/
├── design/
│   ├── icon-spec.md          # 更新：风格 v2.0.0、素材来源、流程
│   ├── mockups/005-icon-set/ # make_mockups.py 重新生成（preview/icon-bar/main-window + 盲测/符合性记录更新）
│   └── ui-guidelines.md      # 如需（图标描述段同步）
├── screenshots/              # update_screenshots.ps1 重新生成
└── README.md                 # 根目录：图标体系描述同步

specs/002-icon-design/contracts/icon-style-spec.md   # 修订至 v2.0.0（实现阶段执行）
```

**Structure Decision**: 不新建任何源文件；改动全部落在既有资源与文档路径内。渲染/校验/打包脚本零改动复用。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

无（Constitution Check 全部通过）。

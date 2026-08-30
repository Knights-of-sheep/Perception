# Implementation Plan: 弹窗样式层次化统一 + 布局设置弹窗优化

**Branch**: `008-unify-dialog-styling` | **Date**: 2026-08-30 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/008-unify-dialog-styling/spec.md`（含 2026-08-30 澄清：布局设置弹窗语义与界面升级）

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

本 feature 含两个工作流：

- **WS1 弹窗样式层次化**（原有范围）：用户反馈弹窗背景与主界面主题背景完全同色（`QDialog` 背景 = `@windowBg@` = 主窗口背景），弹窗在主界面之前"隐身"。为应用弹窗引入独立**弹窗背景层语义色（dialogBg）**：与主界面背景同配色家族、亮度层次不同（深色主题弹窗亮一档、浅色主题暗一档、高对比主题靠边框区分），配合 1px 主题边框形成"悬浮"层次；并为仍使用系统原生标题栏的 `QMessageBox` 提供统一的无边框消息框组件（`ThemedMessageBox`），消除宪法「技术栈约束 · GUI」例外。派生函数保证 25 套主题一致达标，C++ 单测校验亮度差与对比度。
- **WS2 布局设置弹窗优化**（2026-08-30 澄清新增）：用户反馈「按行排 / 按列排」模式不生效（当前与网格视觉无差异）。语义修订为：**按行排 = 全部子窗口一列多行 N×1、按列排 = 一行多列 1×N、网格 = 比例网格 + 行优先/列优先**；行优先/列优先、最大行数/最大列数**仅针对网格模式**且互斥联动显隐（行优先 → 仅显示最大行数，列优先 → 仅显示最大列数）；弹窗界面升级为**分段按钮组** + **实时排列预览** + **恢复默认** + **间隙宽度（0–50px）**。布局计算语义修订落在 `LayoutManager::computeGrid`（纯函数，先红后绿改单测），跨 feature 同步修订 004 spec/data-model。

## Technical Context

**Language/Version**: C++17（MSVC VS2022）；Qt 5.15.2 Widgets + QSS + Fusion 风格——宪法锁定的技术栈，不引入新语言/新版本语法。

**Primary Dependencies**:
- WS1：现有 Qt Widgets/QSS 主题渲染链路（`theme_template.qss` + `theme_catalog_{dark,light,hc}.h` + `ThemeManager::renderQss` token 替换）。
- WS2：`LayoutManager`（纯布局计算，`src/ui/subwindow/layout_manager.{h,cpp}`，仅 QtCore）、`SubwindowContainer::relayout`（QGridLayout + `computeGrid` + `lastCellSpan`，已消费 `cfg.spacing`）、`LayoutSettingsDialog`（弹窗表单）。
- **无新增第三方依赖**（宪法「依赖约束」）。

**Storage**: 不涉及。主题选择持久化走既有 `QSettings`；布局配置（`LayoutConfig`）现状为会话内存态（004 范围未持久化，本次不新增持久化，保持现状）。

**Testing**:
- WS1：`tests/cpp/theme_dialog_layer_test.cpp`——25 套主题 dialogBg 派生（亮度差 ≥ 8pt / text 对比度 ≥ 4.5:1 / HC 边框对比 ≥ 7:1）。
- WS2：`tests/cpp/layout_manager_test.cpp` **语义修订**（红→绿）：Row=N×1、Column=1×N、Grid 比例+优先级、约束轴互斥（`constraintAxis`）、约束仅沿优先级轴生效、间隙 cellSize 扣除、越界/0 窗口边界。
- pytest（`tests/python/`）不涉及（无新增 CLI/命令层接口）。UI 视觉行为以 `quickstart.md` 手动验证矩阵覆盖。

**Target Platform**: Windows 10/11 桌面（VS2022 + Ninja，`scripts/build.ps1`）。

**Project Type**: desktop-app（Qt Widgets，Dock 布局 + QSS 多主题 + 内嵌 Python REPL）。

**Performance Goals**: 布局重排即时生效（现状已满足）；预览绘制为轻量 paintEvent（N≤20 矩形），无热路径；主题渲染一次性毫秒级，无额外运行时开销。

**Constraints**:
- 宪法「技术栈约束 · GUI」：所有应用弹窗 MUST 使用无边框自定义标题栏，禁止系统原生标题栏；QSS `QDialog` 全局规则不得误伤子窗口（已确认应用内 QDialog 仅 4 类弹窗）。
- WS2 语义约束：行优先/列优先/最大行/最大列**仅针对网格**；约束轴 = 优先级轴；Row/Column 模式忽略全部约束与优先级。
- token 单点定义（ui-guidelines §3.1/§4.5）；行为不回归（弹窗功能/拖拽/关闭、子窗口渲染内容与视图状态不变）；合并前 `ctest` + `pytest` 全绿；docs/ 与 README 同步。

**Scale/Scope**:
- WS1：5 类弹窗 × 25 套主题；改动面 `src/ui/theme` + 5 处 QMessageBox 调用点替换 + 新增 ThemedMessageBox/theme_dialog_layer/file_dialog_qss + 1 个测试文件。
- WS2：3 种模式 × 优先级 × 约束 × 间隙（0–50）；改动面 `layout_manager.{h,cpp}`（语义修订 + `constraintAxis`）、`layout_settings_dialog.{h,cpp}`（重构）、新增 `layout_preview_widget.{h,cpp}`、`subwindow_container` 增 `visibleSubwindowCount()`、`MainWindow` 传预览计数、`layout_manager_test.cpp` 修订、004 spec/data-model 同步。

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/008-unify-dialog-styling/spec.md` 已批准且经 2026-08-30 两轮澄清（Clarifications 记录 Q1/Q2，FR-008~015、US4、SC-006/007）（宪法 I / 宪法 VI）
- [x] **Test-First**：WS1 `theme_dialog_layer_test.cpp` 先红后绿；WS2 `layout_manager_test.cpp` 语义修订先红后绿（Row/Column 新期望值、constraintAxis、约束轴用例）；合并前 `ctest` + `pytest` 全绿（宪法 II / 架构契约 Test-First）
- [x] **Layered Core**：改动全部位于 `src/ui`（theme/dialogs/subwindow）与 `tests/cpp`，不触碰 `src/core` 的 model/io/process/event 分层与事件流（宪法 III / 架构契约 Layered Core）
- [x] **Command-Driven**：纯 UI 变更（主题、布局、弹窗样式），宪法明确"纯 UI 变更（布局、主题、面板可见性、选中高亮）除外"；不涉及命令层/数据处理（宪法 IV / 架构契约）
- [x] **Local Design Source**：mockups 无弹窗/布局设置专项视觉稿；WS1 按唯一设计源 `docs/design/ui-guidelines.md` §3.1 扩展 dialog 层；WS2 控件形态沿用 004 既有规范（FR-011 分段按钮组原要求）；不引入 figma 扩展（宪法 V / 架构契约 Local Design Source）
- [x] **Scope**：不涉及任何文件格式（宪法 VI / 架构契约 Data-Visualization Scope）
- [x] **Technology Stack**：Qt 5.15.2 QSS 原生能力 + QGridLayout + QButtonGroup + 自绘 QWidget，无新依赖、无新构建系统（宪法「技术栈约束」）
- [x] **No-Exception（WS2 跨 feature 处置）**：布局语义修订必然触碰 004 已交付代码与测试；处置为"同分支一并修订 004 spec/data-model 与 `layout_manager_test.cpp`"，非"绕过"，符合宪法「测试先行」（修改 004 文档属文档同步，非架构变更，无需宪法修订）

> 任一检查不通过：先修订方案并注明处置，或如实填入下方 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/008-unify-dialog-styling/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── dialog-style-contract.md        # WS1 弹窗样式契约（token 语义 + 消息框 API + SC 映射）
│   └── layout-settings-contract.md     # [新增] WS2 布局设置契约（computeGrid 语义 + 显隐矩阵 + 预览 + 恢复默认 + 间隙）
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
specs/004-dock-layout-manager/
├── spec.md                              # [改] US2 / FR-004~006 / FR-011 / FR-015 语义同步（WS2 跨 feature）
└── data-model.md                        # [改] LayoutConfiguration 语义同步（约束轴、spacing 可配置）

src/ui/
├── theme/
│   ├── theme_template.qss               # [改] 拆分 QDialog 弹窗层规则：@dialogBg@ 背景 + 1px @border@（WS1）
│   ├── theme_types.h                    # [改] ThemeColors 末尾新增 QColor dialogBg（WS1）
│   ├── theme_manager.h/.cpp             # [改] renderQss 改接收 ThemeDescriptor（含 family）；tokens 增加 @dialogBg@（WS1）
│   ├── theme_dialog_layer.h/.cpp        # [新增] deriveDialogBg(windowBg, text, family)（WS1，已实现待合入）
│   └── file_dialog_qss.h/.cpp           # [新增] buildFileDialogQss(colors, dialogBg)（WS1，已实现待合入）
├── themed_message_box.h/.cpp            # [新增] ThemedMessageBox + showThemedMessageBox（WS1，已实现待合入）
├── themed_file_dialog.cpp               # [改] 内嵌 QFileDialog QSS 显式注入（WS1，已实现待合入）
├── dialog_title_bar.h/.cpp              # [不变] 共享标题栏工厂（buildDialogTitleBar）
├── frameless_dialog.h/.cpp              # [不变] 帮助/关于（背景自动随 QSS @dialogBg@）
├── subwindow/
│   ├── layout_manager.h/.cpp            # [改] computeGrid 语义修订（Row=N×1 / Column=1×N / Grid=比例+优先级轴约束）+ 新增 ConstraintAxis 枚举与 constraintAxis()（WS2）
│   ├── layout_settings_dialog.h/.cpp    # [改] 重构：模式改分段按钮组、显隐矩阵、间隙宽度、恢复默认、预览联动（WS2）
│   ├── layout_preview_widget.h/.cpp     # [新增] 实时排列预览（paintEvent 自绘 cell 矩形，复用 computeGrid/cellRect）（WS2）
│   └── subwindow_container.{h,cpp}      # [改] 新增 visibleSubwindowCount()（WS2 预览计数来源；relayout 不改）
├── MainWindow.cpp                       # [改] 2 处 QMessageBox → showThemedMessageBox（WS1）+ openLayoutSettings 传预览计数并订阅 subwindowCountChanged（WS2）
├── log/log_settings_controller.cpp      # [改] 3 处 QMessageBox → showThemedMessageBox（WS1）
└── CMakeLists.txt                       # [改] 注册 themed_message_box / theme_dialog_layer / file_dialog_qss / layout_preview_widget

tests/cpp/
├── theme_dialog_layer_test.cpp          # [新增] 25 套主题 dialogBg 派生校验（WS1，已实现待合入）
├── layout_manager_test.cpp              # [改] 语义修订：Row/Column 单行/单列期望值、Grid 约束轴、constraintAxis、间隙（WS2）
└── CMakeLists.txt                       # [改] 注册/保持新测试

docs/design/ui-guidelines.md             # [改] §3.1 分层色板新增第 5 层 dialog 派生规则（WS1）+ §4 布局弹窗控件规范（WS2）
```

**Structure Decision**: WS2 沿用 004 既有目录（`layout_manager` 纯计算 / `subwindow` 容器与弹窗 / `layout_preview_widget` 归属弹窗组件同层）；弹窗显隐规则以纯函数 `constraintAxis` 落入 `layout_manager`（可单测、UI 与测试共用判定源）。不引入新目录、不改布局架构。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

WS2 触碰 004 已交付代码（`computeGrid` 语义）与测试（`layout_manager_test.cpp` 期望值），属"澄清后语义变更"驱动的既有功能修订，非绕过流程：处置为同分支同步修订 004 spec/data-model/测试并纳入 008 交付，经 Constitution Check「No-Exception」门禁确认。其余无违规。

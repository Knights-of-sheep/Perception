# Implementation Plan: Panel Layout Settings

**Branch**: `010-panel-layout-settings` | **Date**: 2026-08-31 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/010-panel-layout-settings/spec.md`

## Summary

在 Perception 主窗口引入统一的 **Panel Settings（面板布局设置）** 对话框，控制 Data（`fileDock`）、Property（`propertyDock`）、PyShell（`pythonConsoleDock`）三个 `QDockWidget` 面板的布局与显隐。提供**四种预设布局模式**（左右分配 Data/Property 的两种互换 × 底部是否包含 PyShell 的两种组合），三个面板的独立显隐开关（隐藏后其余面板/中央区自动 expand，FR-005），对话框内实时预览 + 主窗口实时生效（FR-006），确认持久化、取消回滚（spec US3 场景 3），布局模式与显隐状态持久化到 `QSettings`（FR-007）。

布局语义以可单测的纯 C++ 类 `PanelLayoutConfig` 为单一事实源（模式 → 面板区域映射、合法组合判定、expand 决策），UI 层 `PanelSettingsDialog` 负责交互，`MainWindow::applyPanelLayout` 将配置应用到三个 dock（`addDockWidget`/`setVisible`/`resizeDocks`）。三个面板已是 `QDockWidget`（004 交付后未变），因此**零 dock 重构**、零新依赖，复用现有 `dialog_title_bar`、QSS 主题与 `view-panel-*` 图标族。

## Technical Context

**Language/Version**: C++17（MSVC/VS2022，/utf-8 /MP12；core 加 /W4 /WX）

**Primary Dependencies**: Qt 5.15.2 Widgets（AUTOMOC/AUTOUIC/AUTORCC）；零新依赖——面板已是 `QDockWidget`，布局经 `QMainWindow` dock API（`addDockWidget`/`removeDockWidget`/`resizeDocks`）实现；不引入 VTK 头、不引入 pybind11

**Storage**: `QSettings`——新增 `panelSettings/mode`、`panelSettings/dataVisible`、`panelSettings/propertyVisible`、`panelSettings/consoleVisible` 四个 key；与现有 `mainWindow/layout`（`saveState/restoreState`）并存；`resetLayout()` 同时清除并重置 panel settings

**Testing**: CTest（`tests/cpp`）——新增纯逻辑单测 `panel_layout_config_test`（模式/区域映射/合法组合/expand 决策，无 GUI 依赖，仿 `layout_manager_test`）+ GUI 交互测试 `panel_settings_dialog_test`（`QApplication`+`QTest`+`QSignalSpy`，仿 `subwindow_view_test`）；不新增 pytest（纯 UI 变更，命令层无改动）

**Target Platform**: Windows 10/11 x64 桌面（Qt 5.15.2 msvc2019_64）

**Project Type**: desktop-app（Qt Widgets 单窗口应用）

**Performance Goals**: 面板隐藏/恢复后布局在 200ms 内完成 expand（SC-002）；对话框内变更在 100ms 内反映到主窗口预览（SC-004）；连续 50 次模式/显隐切换无崩溃卡顿（SC-005 精神）

**Constraints**: Qt 5.15.2 Widgets + QSS + Fusion（深色主题优先）；C++17 禁用 C++20+ 语法；弹窗 MUST 使用无边框自定义标题栏（`dialog_title_bar`，004/008 宪法约束）；`src/ui/theme/icons/icon-map.yaml` 已有 `view-panel-toggle/data/property/console` 图标可复用；不改变三个面板的 widget 内容与停靠语义，仅控制位置与显隐

**Scale/Scope**: 3 面板 × 4 模式 × 8 种显隐组合 × 持久化；`QDockWidget` 数量恒为 3，无新增 dock

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/010-panel-layout-settings/spec.md` 已批准（FR-001~008，checklist 全过）
- [x] **Test-First**：布局语义为纯 C++ 类，Phase 1 给出 `tests/cpp/panel_layout_config_test.cpp` 单测方案；GUI 交互经 `panel_settings_dialog_test.cpp`（QTest）+ quickstart 手动验证
- [x] **Layered Core**：本次改动全部落在 `src/ui`（纯 UI 布局），不触碰 `src/core` 的 model/io/process/event
- [x] **Command-Driven**：布局/面板显隐属宪法 IV 明示豁免的"纯 UI 变更（布局、主题、面板可见性、选中高亮）"，不走命令层；不新增 Python 命令
- [x] **Local Design Source**：`docs/design/mockups/` 无本功能视觉稿（仅 `005-icon-set` 主窗口稿作风格参考，spec Assumptions 确认）；沿用现有 QSS 主题与 `view-panel-*` 图标，无 figma 依赖
- [x] **Scope**：不涉及文件格式读取（io 层零改动），无格式范围问题（宪法 VI 不适用）
- [x] **Technology Stack**：Qt 5.15.2 Widgets / C++17 / CMake+Ninja 合规；零新增依赖；弹窗沿用 `dialog_title_bar` 无边框规范

> 全部通过，无需 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/010-panel-layout-settings/
├── plan.md              # 本文档
├── research.md          # Phase 0：技术选型决策
├── data-model.md        # Phase 1：面板/模式/显隐数据模型
├── quickstart.md        # Phase 1：端到端验证指南
├── contracts/           # Phase 1：PanelLayoutConfig 接口契约 + PanelSettingsDialog 契约
└── tasks.md             # Phase 2（/speckit.tasks 生成，不由此命令创建）
```

### Source Code (repository root)

```text
src/ui/
├── MainWindow.{h,cpp}               # 现有：新增 applyPanelLayout 应用逻辑、启动时恢复 PanelSettings、resetLayout 联动
├── MainWindow_assembly.cpp          # 现有：View 菜单新增 "Panel Settings..." 动作（openPanelSettings）
├── dialog_title_bar.{h,cpp}         # 现有：对话框无边框标题栏复用
├── panellayout/                     # 新增：面板布局设置
│   ├── panel_layout_config.{h,cpp}    # 纯逻辑（QtCore only）：模式枚举、区域映射、合法组合、expand 决策（可单测）
│   ├── panel_preview_widget.{h,cpp}   # 自绘示意图：几何复用 PanelLayoutConfig（仿 004 LayoutPreviewWidget）
│   └── panel_settings_dialog.{h,cpp}  # 对话框：模式选择 + 三面板显隐 toggle + 预览 + 恢复默认 + OK/Cancel
├── theme/theme_template.qss        # 面板设置弹窗 QSS 增补（4.1 覆盖矩阵）
└── subwindow/                      # 现有（004，本次零改动）

tests/cpp/
├── panel_layout_config_test.cpp    # 新增：纯逻辑单测（链接 perception_ui）
└── panel_settings_dialog_test.cpp  # 新增：GUI 交互测试（QTest + QSignalSpy，PERCEPTION_BUILD_GUI 块内）
```

**Structure Decision**: 布局语义与 UI 分离——`PanelLayoutConfig` 为纯 C++ 类（输入：模式 + 三面板显隐 → 输出：每个 dock 的目标区域与可见性、合法组合校验、expand 决策表），仅依赖 QtCore 枚举，使 Test-First 可行（`tests/cpp` 无窗口环境可测）。`MainWindow::applyPanelLayout` 为薄应用层（`removeDockWidget`→`addDockWidget`→`setVisible`→`resizeDocks`），因需访问 MainWindow 私有 dock 成员而保留为主窗口方法（dock 摆放仅数行、非独立类）。`PanelSettingsDialog` 仿 004 `LayoutSettingsDialog`：无边框标题栏 + 信号即时生效 + 自绘预览；差异在于本功能需 OK/Cancel（spec US3 场景 3：Cancel 回滚），故对话框持有打开时快照，Cancel 恢复快照、OK 持久化。持久化经 `QSettings` 四个新 key，`resetLayout()` 同步重置（保证"Reset Layout"语义完整）。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

本功能全部检查通过，无违规，无需填写。

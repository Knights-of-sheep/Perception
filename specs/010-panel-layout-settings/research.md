# Research: Panel Layout Settings

> Phase 0 输出。输入：代码库探索报告 + 规格约束（spec FR-001~008 / SC-001~005）。

## 1. 布局应用机制

- **Decision**: 沿用 `QMainWindow` dock API（`removeDockWidget` → 按模式 `addDockWidget` 到目标区域 → `setVisible` → `resizeDocks` 规整），`MainWindow::applyPanelLayout` 为薄应用层。
- **Rationale**: 探索确认三个面板已是 `QDockWidget`（`fileDock`=左/Data、`propertyDock`=右/Property、`pythonConsoleDock`=底/PyShell），`QMainWindow` 原生支持多区域 dock、隐藏自动 expand、`saveState/restoreState` 持久化。模式切换 = 重新摆放 dock，零 dock 重构、零新依赖，且与现有显隐 toggle（`toggleFileDockAction_` 等）、`resetLayout()`、全屏（`setContainerFullscreen` 隐藏三 dock）天然兼容。
- **Alternatives considered**:
  - 自研容器/`QSplitter` 重构：004 已证明 dock 为产品根基（宪法「技术栈约束 · GUI」），重构风险高、收益低 —— 放弃。
  - 只靠 `saveState/restoreState` 记忆 dock 位置、不显式保存"模式"：用户手动拖拽后模式语义丢失，无法满足 FR-007"布局模式持久化" —— 放弃。

## 2. 模式与显隐语义

- **Decision**: 四种模式 = **左右分配（Data↔Property 互换，2 种）× 底部是否包含 PyShell（2 种）** 的完整预设；三个面板的显隐 toggle 独立覆盖模式默认值。任何"模式 × 显隐"组合均为合法布局（FR-008 自动满足：模式保证左右各一、底部唯一为 PyShell）。
- **Rationale**: 用户提供图片为 2×2 组合（左右互换 × 底部有无）；spec FR-002 要求四种模式、FR-004 要求独立显隐。将"无底部模式"解释为"PyShell 默认隐藏"而非"禁止 PyShell"——若用户在无底部模式下显式显示 PyShell，PyShell 停靠底部（其唯一合法区域），不产生非法布局，符合 FR-008"自动纠正到最近合法状态"。
- **Alternatives considered**:
  - 模式禁止覆盖（无底部模式下禁显 PyShell）：与"独立显隐"语义冲突，用户必须切模式才能显示控制台 —— 放弃。
  - 模式仅含左右分配（2 种）+ 底部显隐由 toggle 单独表达：与"四种模式"的字面要求不符 —— 放弃。

## 3. Expand 行为（FR-005）

- **Decision**: 不自定义 expand 算法，依赖 `QMainWindow` dock 空间分配（隐藏 dock 后空间自动由中央区与相邻 dock 吸收）+ `applyPanelLayout` 末尾 `resizeDocks` 规整比例。expand 决策表（哪些区域吸收释放空间）由 `PanelLayoutConfig` 以纯逻辑形式暴露，供单测断言。
- **Rationale**: QMainWindow 对 dock 隐藏/显示的空间再分配是既有、稳定行为（004/005 已依赖），自定义 expand 反而引入重复且易偏离；"相关面板视情况 expand"（spec）的语义 = 可见面板 + 中央区填充释放空间，QMainWindow 天然满足。
- **Alternatives considered**: 手工计算每面板 `resize` 分配 —— 与 QMainWindow 布局引擎冲突、resize 事件处理繁琐 —— 放弃。

## 4. 预览与回滚（FR-006 / spec US3 场景 3）

- **Decision**: 对话框打开时快照（模式 + 三面板显隐）；任何变更即时经 `applyPanelLayout` 应用到主窗口（实时预览）；OK = 持久化到 `QSettings`；Cancel = 恢复快照并重新应用；"恢复默认" = 默认模式（左 Data 右 Property + 底部 PyShell）+ 三面板全显。
- **Rationale**: spec FR-006 要求"主窗口实时预览"，US3 场景 3 要求 Cancel 回滚——快照 + 重放是最短可靠路径；恢复默认语义与 `resetLayout()` 现有默认布局（004 交付）保持一致，保证两入口不打架。
- **Alternatives considered**: 预览仅在对话框内小图展示、主窗口不动 —— 不满足 FR-006 主窗口预览 —— 放弃。

## 5. 持久化（FR-007）

- **Decision**: `QSettings` 新增 `panelSettings/mode`、`panelSettings/dataVisible`、`panelSettings/propertyVisible`、`panelSettings/consoleVisible` 四 key；启动时读取并应用；`resetLayout()`（Ctrl+Shift+L）同时清除四 key 并重置为默认（现有 `mainWindow/layout` 移除逻辑不变）。
- **Rationale**: 现有 `saveState/restoreState` 只记 dock 物理状态、不记"模式"语义；显式保存模式 + 显隐使重启后布局确定性地回到预设（FR-007/SC-005）。`resetLayout` 联动保证"重置布局"对用户语义完整（面板也回默认）。
- **Alternatives considered**: 仅依赖 `saveState/restoreState`：用户手动拖拽后重启仍保留手动位置，模式语义漂移 —— 放弃。

## 6. 测试策略（宪法 II Test-First）

- **Decision**: 两个新增测试文件——
  - `tests/cpp/panel_layout_config_test.cpp`：纯逻辑（链接 `perception_ui`，无 GUI），覆盖模式→区域映射、合法组合判定（FR-003/FR-008）、expand 决策（FR-005）、默认值与持久化字段序列化；
  - `tests/cpp/panel_settings_dialog_test.cpp`：GUI 交互（`QApplication`+`QTest`+`QSignalSpy`，`PERCEPTION_BUILD_GUI` + `Qt5Test_FOUND` 块内，仿 `subwindow_view_test`），覆盖打开对话框、切换模式发信号、显隐 toggle 发信号、预览 widget 状态、OK/Cancel 行为。
- **Rationale**: 布局语义为确定性计算，纯逻辑可穷举 4 模式 × 8 显隐组合；GUI 层交互沿用 `subwindow_view_test` 已验证的测试范式。
- **Alternatives considered**: 仅手动验证 —— 违反宪法 II 且无法覆盖全组合 —— 放弃。

## 7. 入口与图标

- **Decision**: "视图"菜单新增 "Panel Settings..." 动作（位于 "Layout Settings..." 附近），复用 `icon-map.yaml` 现有 `view-panel-toggle` 图标（已确认存在）；对话框采用 `dialog_title_bar` 无边框自定义标题栏。
- **Rationale**: spec FR-001 指明 View 菜单入口；图标零新增成本（`view-panel-*` 图标族已就位）；弹窗规范（无边框自定义标题栏）为宪法强制（004/008）。
- **Alternatives considered**: 工具栏按钮 / 设置页签 —— spec 未要求，本次不做。

# 契约: PanelSettingsDialog

> 内部接口契约：供 `tests/cpp` GUI 测试与 `src/ui` 实现使用。对应 spec FR-001、FR-002、FR-004、FR-006 及 spec US3。

## 1. 入口与打开方式

- 主窗口"视图"菜单新增动作 **"Panel Settings..."**（`openPanelSettings`，复用 `view-panel-toggle` 图标），打开 `PanelSettingsDialog`。
- 对话框为无边框 + 自定义标题栏（`dialog_title_bar`，图标+标题+关闭按钮，可拖拽移动），与 004/008 弹窗规范一致。

## 2. 控件契约

| 控件 | 类型 | 行为 |
|------|------|------|
| 布局模式 | 4 个 checkable `QPushButton`（`QButtonGroup` exclusive，objectName `panelModeButton`） | 四种模式（DualOnly / DualWithConsole / DualReversedOnly / DualReversedWithConsole），点击即切换并即时应用（FR-002） |
| Data 面板显隐 | `QCheckBox`（objectName `panelDataVisibleCheck`） | FR-004，切换即时应用 |
| Property 面板显隐 | `QCheckBox`（objectName `panelPropertyVisibleCheck`） | FR-004，切换即时应用 |
| PyShell 面板显隐 | `QCheckBox`（objectName `panelConsoleVisibleCheck`） | FR-004，切换即时应用 |
| 布局示意图 | 自绘 `PanelPreviewWidget`（objectName `panelPreviewWidget`） | 几何复用 `PanelLayoutConfig`（targetArea + isPanelVisible），实时反映当前模式 × 显隐；左/右/底/中央区以主题色块表达 |
| 恢复默认 | `QPushButton`（objectName `panelResetDefaultsButton`） | 恢复 `DualWithConsole` + 三面板全显，即时预览，不自动关闭 |
| 确认 | `QPushButton`（objectName `panelOkButton`） | OK：持久化当前配置到 `QSettings` 并关闭（FR-007） |
| 取消 | `QPushButton`（objectName `panelCancelButton`） | Cancel：恢复打开时快照并关闭（spec US3 场景 3） |

## 3. 信号与行为

- `configChanged(PanelLayoutConfig)`：模式/显隐任一变更即发射，MainWindow 槽 `applyPanelLayout` 实时应用到主窗口（FR-006 预览；SC-004 <100ms）。
- 打开时以当前主窗口配置初始化控件；快照 `cfg0` 保存于对话框。
- OK/Cancel 之外关闭（标题栏关闭按钮）按 Cancel 语义处理（回滚快照）。

## 4. 测试约定（tests/cpp/panel_settings_dialog_test.cpp）

- `QSignalSpy` 断言：切换模式/勾选显隐 → `configChanged` 发射且负载正确。
- 控件定位：`findChild<QPushButton*>("panelModeButton")`、`findChild<QCheckBox*>("panelDataVisibleCheck")` 等（仿 `subwindow_view_test`）。
- OK/Cancel 行为：OK 后 `QSettings` 四 key 更新；Cancel 后主窗口恢复快照（经信号/状态断言）。

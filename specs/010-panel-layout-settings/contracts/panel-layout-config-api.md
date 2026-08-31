# 契约: PanelLayoutConfig 接口与布局规则

> 内部接口契约：供 `tests/cpp` 单测与 `src/ui` 实现使用。对应 spec FR-001~008。

## 1. PanelLayoutConfig（纯 C++，仅依赖 QtCore 枚举，可无窗口单测）

```cpp
namespace perception::ui {

enum class PanelId { Data, Property, PyShell };

enum class DockArea { Left, Right, Bottom };

enum class PanelLayoutMode {
  DualOnly,                 // 左=Data 右=Property，PyShell 非全尺寸（嵌入 Plot 下方窄条）
  DualWithConsole,          // 左=Data 右=Property，PyShell 全尺寸（底部全宽，默认）
  DualReversedOnly,         // 左=Property 右=Data，PyShell 非全尺寸（嵌入 Plot 下方窄条）
  DualReversedWithConsole,  // 左=Property 右=Data，PyShell 全尺寸（底部全宽）
};

struct PanelLayoutConfig {
  PanelLayoutMode mode = PanelLayoutMode::DualWithConsole;
  bool dataVisible = true;      // FR-004
  bool propertyVisible = true;  // FR-004
  bool consoleVisible = true;   // FR-004
};

// FR-003：面板在指定模式下的目标区域
DockArea targetArea(PanelLayoutMode mode, PanelId id);

// FR-003/FR-008：区域合法性（Data/Property → Left|Right；PyShell → Bottom）
bool isLegalArea(PanelId id, DockArea area);

// FR-002：模式对应的底部 PyShell 默认显隐（四种模式均含 PyShell，默认显示）
bool modeDefaultsConsoleVisible(PanelLayoutMode mode);

// FR-002：模式是否采用全尺寸（底部全宽）PyShell（*WithConsole=true；*Only=false）
bool modeHasFullWidthConsole(PanelLayoutMode mode);

// FR-005：expand 决策——隐藏某面板后，哪些区域吸收释放空间
// 返回每个面板在目标显隐下的可见性（依据 cfg 与区域约束）
bool isPanelVisible(const PanelLayoutConfig& cfg, PanelId id);

// FR-008：任何「模式 × 显隐」组合恒合法（模式预设保证区域合法）
bool isLegalConfig(const PanelLayoutConfig& cfg);

// FR-007：四字段与 QSettings key 的双向转换（panelSettings/mode 等）
QString modeToKey(PanelLayoutMode mode);
PanelLayoutMode modeFromKey(const QString& key, PanelLayoutMode fallback);

}  // namespace perception::ui
```

## 2. 规则

- **targetArea**：`DualOnly/DualWithConsole` → Data=Left、Property=Right；`DualReversedOnly/DualReversedWithConsole` → Data=Right、Property=Left；PyShell 恒 = Bottom（FR-003）。
- **isLegalArea**：PyShell ∉ {Left, Right}；Data/Property 均 ∈ {Left, Right}（FR-003 约束）。
- **modeDefaultsConsoleVisible**：四种模式均返回 true——模式预设下 PyShell 默认显示，仅尺寸形态不同（用户示意图 2026-08-31：`*Only` 非全尺寸 / `*WithConsole` 全尺寸）。
- **modeHasFullWidthConsole**：`DualWithConsole`/`DualReversedWithConsole` → true（PyShell 横跨底部全宽，两侧面板在其上方）；`DualOnly`/`DualReversedOnly` → false（PyShell 嵌入 Plot 下方窄条，两侧面板保持全高）。
- **isPanelVisible**：Data/Property 取 `cfg.dataVisible/propertyVisible`；PyShell 取 `cfg.consoleVisible`（任意模式 + 用户显式显示 PyShell = 按其尺寸形态落位，见 isLegalConfig 恒合法）。
- **isLegalConfig**：恒 true（模式为预设组合，显隐不破坏区域约束；FR-008 由此满足，测试锁定防回归）。
- 非法输入（如未知模式 key）：`modeFromKey` 回退 `fallback`（默认 `DualWithConsole`）。

## 3. 应用层（MainWindow::applyPanelLayout）

- 输入：`PanelLayoutConfig`。
- 行为：PyShell 先按尺寸形态定宿主（`setConsoleEmbedded`：非全尺寸 *Only → 中央下方嵌入式槽位；全尺寸 *WithConsole → 底部 dock）→ `removeDockWidget` 后按 `targetArea` `addDockWidget`（PyShell 仅在非嵌入 + 可见时加入底部 dock）→ 按 `isPanelVisible` `setVisible`（嵌入式 PyShell 显隐作用于嵌入式槽位）→ `resizeDocks` 规整尺寸（隐藏面板后可见面板与中央区按比例吸收空间，FR-005）。
- 约束：不改变 dock 内容、objectName、标题；不销毁 dock；全屏态（`setContainerFullscreen`）期间调用需先退出全屏或兼容处理（与现有全屏恢复逻辑协同）。

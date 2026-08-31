# 数据模型: Panel Layout Settings

> Phase 1 输出。对应 spec FR-001~008；描述实体、关系、布局规则与持久化字段。

## 1. 实体

### Panel（面板）

| 字段 | 类型 | 说明 |
|------|------|------|
| id | enum { Data, Property, PyShell } | 逻辑标识（spec 语义：data / property / pyshell） |
| dockObjectName | string | 现有 dock 对象名：`fileDock` / `propertyDock` / `pythonConsoleDock` |
| 合法区域 | enum { Left, Right, Bottom } | 约束：Data/Property → {Left, Right}；PyShell → {Bottom}（FR-003） |
| visible | bool | 显隐状态（FR-004） |

生命周期：三个面板恒存在（不随本功能创建/销毁）；显隐仅影响 view 状态，不销毁 dock 与其内容（隐藏不丢失内容，spec Assumptions）。

### PanelLayoutMode（布局模式，FR-002）

| 值 | 左 | 右 | PyShell 尺寸形态 |
|----|----|----|------------------|
| DualOnly | Data | Property | 非全尺寸（嵌入 Plot 下方窄条，两侧面板保持全高） |
| DualWithConsole | Data | Property | 全尺寸（底部全宽，两侧面板在其上方） |
| DualReversedOnly | Property | Data | 非全尺寸（嵌入 Plot 下方窄条，两侧面板保持全高） |
| DualReversedWithConsole | Property | Data | 全尺寸（底部全宽，两侧面板在其上方） |

> 四种模式均含 PyShell（用户示意图 2026-08-31），仅尺寸形态不同；弹窗按钮排列：上排 = 非全尺寸（Dual / Reversed），下排 = 全尺寸（Dual + Console / Reversed + Console）。
>
> 默认模式 = `DualWithConsole`（与应用当前布局一致：左 Data 右 Property + 底部 PyShell，向后兼容）。

### PanelLayoutConfig（面板布局配置，单一事实源）

| 字段 | 类型 | 默认 | 说明 |
|------|------|------|------|
| mode | PanelLayoutMode | DualWithConsole | 预设布局模式（FR-002） |
| dataVisible | bool | true | Data 面板显隐（FR-004） |
| propertyVisible | bool | true | Property 面板显隐（FR-004） |
| consoleVisible | bool | true | PyShell 面板显隐（FR-004） |

## 2. 关系

- MainWindow 1 ── 3 Panel（dock 成员：`fileDock_` / `propertyDock_` / `pythonDock_`）
- MainWindow 1 ── 1 PanelLayoutConfig（单一全局配置，经 `QSettings` 持久化）
- PanelLayoutMode 1 ── N Panel（模式定义每个面板的默认区域与默认显隐）

## 3. 布局规则（PanelLayoutConfig / MainWindow::applyPanelLayout）

输入：`cfg` = PanelLayoutConfig。

- **区域分配（FR-003）**：模式决定左右面板身份——`DualOnly/DualWithConsole` 左=Data、右=Property；`DualReversedOnly/DualReversedWithConsole` 左=Property、右=Data。任一时刻左右各至多一个面板；底部仅 PyShell。
- **PyShell 尺寸形态（FR-002）**：`*WithConsole`（全尺寸）→ PyShell 停靠底部 dock，横跨全宽，两侧面板在其上方；`*Only`（非全尺寸）→ PyShell 嵌入中央区下方窄条（`setConsoleEmbedded`，只占中央宽度、两端抵在两侧面板），两侧面板保持全高。嵌入式窄条（`consoleEmbeddedHost_`）自上而下：**可拖拽分隔条**（`EmbeddedConsoleSash`，6px，与 Plot 的分界；绘制与 dock separator 同布局的 6 个点 handle，鼠标拖拽调整 PyShell 高度，hover/拖拽中 accent 高亮）→ **标题栏**（复用 `dockTitleBar`/`dockTitleLabel` QSS，带与 DockTitleBar 同款功能按钮：最大化/恢复 = 嵌入式 ↔ 全尺寸底部 dock 切换 `setConsoleFullWidth`、浮动 = 分离独立窗口、关闭 = 隐藏）→ PyShell 内容。PyShell 内容（PythonConsole + 操作按钮栏）在 dock 与嵌入式槽位间 reparent，内容不丢失。**全宽覆盖（临时态，不持久化、不改变模式）**：`setConsoleFullWidth(true)` 时内容入 dock、嵌入式隐藏，pythonDock 标题栏最大化为"恢复嵌入式"（`DockTitleBar::setFullWidthConsole`），点击收回嵌入式；任何 `applyPanelLayout`（模式重排/预览/OK）会清除该覆盖态。
- **显隐与 expand（FR-004/FR-005）**：
  - 面板 `visible=false` → 该 dock `setVisible(false)`（嵌入式 PyShell → 嵌入式槽位隐藏），其空间由可见面板与中央区吸收（QMainWindow 天然分配 + `resizeDocks` 规整）。
  - 左右同隐 → 中央区占满整个工作区宽度；底部隐 → 中央区占满高度；全隐 → 中央区占满全部可用区域。
  - 恢复显隐 → 按模式区域重新摆放（`addDockWidget`）并可见。
- **合法组合（FR-008）**：模式为预设组合（左右各一、底部仅 PyShell），显隐不产生非法布局；任何「模式 × 显隐」组合恒合法，无需纠正。用户显式显示 PyShell 时按模式尺寸形态落位（全尺寸 → 底部 dock；非全尺寸 → 嵌入中央下方），不破坏左右分配。
- **默认值**：`DualWithConsole` + 三面板全显（与 `resetLayout()` 默认一致，008 精神：恢复默认 = 明确、可一键到达）。

## 4. 状态迁移与预览

- **编辑态（FR-006）**：对话框打开 → 快照 `cfg0`；任何变更即时应用（主窗口实时预览）；OK → 持久化并关闭；Cancel → 恢复 `cfg0` 重新应用并关闭；"恢复默认" → 设为默认配置并即时预览（不自动关闭）。
- **运行态**：启动时从 `QSettings` 恢复四字段并应用（FR-007/SC-005）。

## 5. 持久化字段（QSettings）

| key | 值域 | 默认 |
|-----|------|------|
| `panelSettings/mode` | DualOnly / DualWithConsole / DualReversedOnly / DualReversedWithConsole | DualWithConsole |
| `panelSettings/dataVisible` | bool | true |
| `panelSettings/propertyVisible` | bool | true |
| `panelSettings/consoleVisible` | bool | true |

`resetLayout()` 移除 `mainWindow/layout` 的同时清除四 key 并应用默认配置。

## 6. 验证规则汇总（来自 spec）

- 四种模式切换后左右面板身份与 PyShell 尺寸形态符合模式定义（FR-002/SC-001）：全尺寸 → 底部全宽、两侧面板在其上方；非全尺寸 → 嵌入 Plot 下方窄条、两侧面板保持全高
- 隐藏任一面板后无空白死区、无重叠，其余面板/中央区 expand（FR-005/SC-003）
- 任意模式 × 显隐组合不产生非法布局（FR-008）
- 对话框内变更主窗口 100ms 内反映（SC-004）；隐藏/恢复后 200ms 内完成 expand（SC-002）
- 重启后恢复上次模式与显隐（FR-007/SC-005）
- Cancel 后主窗口回到打开前布局（spec US3 场景 3）

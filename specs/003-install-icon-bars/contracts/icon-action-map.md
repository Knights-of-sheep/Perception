# Contract: 图标-动作映射（Icon-Action Map）

**Feature**: 003-install-icon-bars | **Version**: 1.0.0 | **Source**: spec.md FR-001~011、research.md 决策 1-6

> 本契约是「动作 ↔ 图标 ↔ 状态 ↔ 归属」的**唯一可检查定义**，与 `tests/cpp/icon_action_map_test.cpp` 同源。任一动作与下表不一致即计为不符合项。
>
> 图标 id 均指 `src/ui/theme/icons/icon-map.yaml` 中已登记语义；新增图标见 [research.md 决策 2](../research.md)。

## 1. 菜单项图标映射（FR-002）

| 菜单 | 菜单项 | 动作 id | 图标 id | 备注 |
|---|---|---|---|---|
| 文件 | 打开文件… | action.openFile | `file-open` | 快捷键已有 |
| 文件 | 导出命令脚本… | action.exportCommands | `file-export-data` | 既有动作，禁用态保持 |
| 文件 | 导出主界面图片… | action.exportImage | `file-export-screenshot` | |
| 文件 | 退出 | action.exit | `file-close` | |
| 编辑 | 撤销 | action.undo | `edit-undo` | 新增，禁用态（FR-011） |
| 编辑 | 重做 | action.redo | `edit-redo` | 新增，禁用态（FR-011） |
| 视图 | 数据面板 | action.toggleFileDock | `view-panel-toggle` | checkable |
| 视图 | 属性面板 | action.togglePropertyDock | `view-panel-toggle` | checkable |
| 视图 | Python 控制台 | action.togglePythonConsole | `view-panel-toggle` | checkable |
| 视图 | 重置布局 | action.resetLayout | `view-reset-camera` | 语义「重置」，tooltip 区分 |
| 主题 | （动态主题列表 15 项） | — | — | 状态列表，不配图标（research 决策 1） |
| 设置 | 日志级别（5 项）/ 全部启用 / 全部禁用 | — | — | 状态列表，不配图标 |
| 设置 | VTK 日志拦截 | action.vtkLog | `tools-settings` | |
| 设置 | 打开日志目录 | action.openLogDir | `tools-settings` | |
| 设置 | 设置日志路径… | action.setLogPath | `tools-settings` | |
| 设置 | 清除历史日志 | action.clearLog | `edit-delete-selection` | |
| 帮助 | 帮助 | action.help | `tools-help` | |
| 帮助 | 关于 | action.about | `tools-about` | |

## 2. 左侧功能栏（FR-003，顺序固定）

| # | 按钮 | 动作 id | 图标 id | 状态 | 归属联动 |
|---|---|---|---|---|---|
| 1 | 撤销 | action.undo | `edit-undo` | disabled（FR-011） | 编辑菜单同动作 |
| 2 | 重做 | action.redo | `edit-redo` | disabled（FR-011） | 编辑菜单同动作 |
| 3 | 加载文件 | action.openFile | `file-open` | enabled | 文件菜单同动作（FR-004） |
| 4 | 加载脚本 | action.loadScript | `file-load-script` | disabled（FR-011） | 新增动作 |
| 5 | 主界面截图 | action.exportImage | `file-export-screenshot` | enabled | 文件菜单同动作（FR-004） |
| 6 | 主界面视频录制 | action.recordScreen | `file-record-screen` | disabled（FR-011） | 新增动作 |
| 7 | 刷新 | action.refresh | `view-refresh` | disabled（FR-011） | 新增动作 |
| 8 | 数据面板显隐 | action.toggleFileDock | `view-panel-toggle` | enabled, checkable | 视图菜单同动作；选中态=面板可见 |
| 9 | 属性面板显隐 | action.togglePropertyDock | `view-panel-toggle` | enabled, checkable | 视图菜单同动作；选中态=面板可见 |
| 10 | 命令窗口显隐 | action.togglePythonConsole | `view-panel-toggle` | enabled, checkable | 视图菜单同动作；选中态=面板可见 |

## 3. 右侧功能栏（FR-005，顺序固定）

| # | 按钮 | 动作 id | 图标 id | 状态 |
|---|---|---|---|---|
| 1 | 放大 | action.zoomIn | `view-zoom-in` | disabled（FR-011） |
| 2 | 缩小 | action.zoomOut | `view-zoom-out` | disabled（FR-011） |
| 3 | 自适应显示 | action.fitView | `view-fit-screen` | disabled（FR-011） |
| 4 | 重置视图 | action.resetView | `view-reset-camera` | disabled（FR-011） |
| 5 | 添加曲线 | action.addCurve | `analysis-curve-add` | disabled（FR-011） |
| 6 | 移除曲线 | action.removeCurve | `analysis-curve-remove` | disabled（FR-011） |
| 7 | 数据提取 | action.extractData | `analysis-extract` | disabled（FR-011） |
| 8 | 坐标轴设置 | action.axisSettings | `analysis-axis-settings` | disabled（FR-011） |
| 9 | 图例 | action.toggleLegend | `analysis-legend` | disabled（FR-011） |

## 4. 新增图标清单（research 决策 2）

| 图标 id | category | 语义 | 服务按钮 |
|---|---|---|---|
| `file-load-script` | file | 加载脚本 | 左侧 #4 加载脚本 |
| `file-record-screen` | file | 主界面视频录制 | 左侧 #6 视频录制 |
| `view-refresh` | view | 刷新 | 左侧 #7 刷新 |
| `view-panel-toggle` | view | 面板显隐（通用） | 左侧 #8/#9/#10 与视图菜单三开关 |

> 全部新增图标须通过 `scripts/check_icons.py`（icon-style-spec 全部条款）并登记入 `icon-map.yaml`（SC-006）。

## 5. 五态与样式约定

| 状态 | 图标呈现 | 容器（QSS） | 依据 |
|---|---|---|---|
| normal | 原始 PNG（FG_TEXT） | 透明 | T-01 |
| hover | 原始 PNG | BG_CONTROL 圆角 4px 底 | T-02 |
| pressed | FG_TEXT_WEAK 派生 | BG_CONTROL 加深 | T-03 |
| disabled | FG_TEXT_DISABLED 40% 派生 | 透明 | T-04 |
| selected（checkable） | ACCENT 派生 | SELECTION_BG 圆角 4px 底 | T-05/T-06 |

- 派生变体由 `IconFactory`（src/ui/theme/）运行时生成（research 决策 3）；disabled/selected 必须双通道可辨（T-06）
- 功能栏按钮为 `ToolButtonIconOnly`，iconSize 24；悬停显示 tooltip（FR-006）
- 菜单图标使用同一 PNG 资源（QIcon 自动选择最接近尺寸，菜单按 16px 呈现）

## 6. 契约校验入口

- `tests/cpp/icon_action_map_test.cpp`：断言 §1~§3 表格完整性与 §5 状态规则（V-1~V-8）
- `scripts/check_icons.py`：断言 §4 新增图标符合性
- `scripts/build.ps1 -Gui`：qrc 资源编译验证

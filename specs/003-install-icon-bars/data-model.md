# Data Model: 安装程序图标与功能图标

**Feature**: 003-install-icon-bars | **Date**: 2026-08-25 | **Phase**: 1（Design & Contracts）

> 本功能为 UI 变更，不涉及核心数据层持久化。「数据模型」指 UI 层实体及其关系，作为 [contracts/icon-action-map.md](contracts/icon-action-map.md) 契约与 `tests/cpp/icon_action_map_test.cpp` 单测的依据。

## 1. 实体

### 1.1 UiAction（UI 动作）

菜单项与功能栏按钮共用的逻辑单元（对应 Qt QAction 的抽象）。

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 唯一标识（kebab-case），如 `action.openFile` |
| text | string | 显示文字（菜单项/功能栏 tooltip 基准） |
| tooltip | string | 悬停提示，必须非空（FR-006） |
| shortcut | string? | 键盘快捷键（可空） |
| iconId | string | 引用的 IconAsset.id（FR-002/003/005） |
| checkable | bool | 是否为可勾选动作（面板显隐开关 = true） |
| enabled | bool | 初始可用状态（未实现功能 = false，FR-011） |
| kind | set<Kind> | 归属：`menu` / `leftToolBar` / `rightToolBar`（一个动作可多处归属，如 打开文件=menu+leftToolBar） |

**状态规则（来自 spec FR）**：
- `enabled=false` 的动作必须不连接任何功能槽（无副作用）
- 同一动作多归属时行为必须一致（FR-004）
- checkable 动作的选中态必须反映对应面板可见性（FR-003 场景 3）

### 1.2 IconAsset（图标资源）

由 002 交付的图标资产 + 本次新增 4 枚。

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | kebab-case `<功能区>-<功能>`（N-01），全库唯一（N-02） |
| category | enum | file / edit / view / analysis / animation / tools |
| semantic | string | 语义描述（icon-map.yaml 登记） |
| sizes | [16, 24, 32] | 渲染档位（G-02） |
| svgSrc | path | `src/ui/theme/icons/actions/<id>.svg` |
| pngQrc | string | `:/perception/icons/icons/png/actions/<id>-24.png` 等 |
| states | set | {normal, hover, pressed, disabled, selected}，disabled/selected 可运行时派生（T-07） |

**校验规则**：新增图标必须通过 `scripts/check_icons.py`（风格/网格/色板/命名/映射，icon-style-spec 全部条款）；SVG 源与三档 PNG 必须齐备（A-03）。

### 1.3 ToolBar（功能栏）

| 字段 | 类型 | 说明 |
|---|---|---|
| side | enum | left / right |
| objectName | string | `leftToolBar` / `rightToolBar`（QSS 选择器锚点） |
| orientation | enum | vertical（FR：纵向布局） |
| iconSize | int | 24（QSS 五态容器基准） |
| actions | ordered list<UiAction.id> | 按钮顺序（FR-003 集合固定） |

**规则**：左侧功能栏 actions 集合恰好 = {撤销, 重做, 加载文件, 加载脚本, 主界面截图, 主界面视频录制, 刷新, 数据面板显隐, 属性面板显隐, 命令窗口显隐}（FR-003）；右侧恰好 = {放大, 缩小, 自适应显示, 重置视图, 添加曲线, 移除曲线, 数据提取, 坐标轴设置, 图例}（FR-005）。

## 2. 关系

```text
UiAction 1 ─── 1 IconAsset        (UiAction.iconId → IconAsset.id，非空)
ToolBar 1 ─── * UiAction          (功能栏按钮有序集合；同一 UiAction 可属于 menu 与 ToolBar)
Kind.menu 覆盖全部菜单操作项      (FR-002：菜单操作项均有 iconId)
```

## 3. 状态迁移

- 面板开关动作（checkable）：`unchecked ↔ checked`，由用户点击触发；checked 状态 = 面板可见，程序内部面板显隐变更（如标题栏按钮）必须同步动作状态
- 未实现动作（enabled=false）：状态固定为 disabled，不迁移，直到对应功能 spec 将其启用
- 五态图标呈现：normal/hover/pressed/disabled/selected 由 QSS 容器 + 图标派生共同完成（research 决策 3），IconFactory 派生 disabled/selected 变体

## 4. 验证规则汇总（单测断言来源）

| 编号 | 规则 | 来源 |
|---|---|---|
| V-1 | 左侧功能栏集合 = 10 个固定 id，顺序固定 | FR-003 |
| V-2 | 右侧功能栏集合 = 9 个固定 id，顺序固定 | FR-005 |
| V-3 | 每个菜单操作项 iconId 非空 | FR-002 |
| V-4 | 每个动作 tooltip 非空 | FR-006 |
| V-5 | 每个动作 iconId 对应 PNG（16/24/32）资源存在 | FR-008 |
| V-6 | 未实现动作 enabled=false 且无槽连接 | FR-011 |
| V-7 | 三个面板开关 checkable=true | FR-003 |
| V-8 | 左侧功能栏可用按钮（打开文件/截图/三面板开关）与对应菜单动作行为一致 | FR-004 |

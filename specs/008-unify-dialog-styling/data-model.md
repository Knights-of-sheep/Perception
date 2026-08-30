# Data Model: 弹窗样式层次化统一

**Date**: 2026-08-30 | **Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md) | **Research**: [research.md](research.md)

> 本 feature 为纯 UI 样式层变更，不涉及持久化数据；本文件记录**设计模型**（层级背景模型、派生规则、弹窗类型清单、消息框组件模型），供实现与验收对照。

## 1. 主题背景层级模型（5 层）

弹窗层次化改造后，主题背景形成明确的层级序列（深色经典示例值）：

| 层级 | 语义 token | 用途 | 来源 | 深色示例 |
|---|---|---|---|---|
| L0 | `windowBg` | 主窗口背景 | 显式（25 套色板） | `#1E1E1E` |
| L1 | `panelBg` | 菜单栏 / 工具栏 / Dock / 列表底 | 显式 | `#252526` |
| L2 | `controlBg` | 按钮 / 输入框 / 控件底 | 显式 | `#3C3C3C` |
| L3 | `viewBg` | 中央视图底（绘图区） | 显式 | `#161616` |
| **L4** | **`dialogBg`** | **弹窗背景层（新增）** | **派生（R1）** | **约 `#333333`** |

**层次关系约束**：
- 深色族：`viewBg ≤ windowBg < panelBg < dialogBg < controlBg`（弹窗高于所有界面层级，悬浮）
- 浅色族：`dialogBg < windowBg`（弹窗比主界面暗一档，形成浮起感）
- 高对比族：`dialogBg == windowBg`，层次由边框达成

## 2. dialogBg 派生规则

`deriveDialogBg(windowBg, text, family) -> QColor`（输入无效色时返回无效色，由 renderQss 回退；`text` 用于对比度保护）：

| family | 规则 | 约束 |
|---|---|---|
| Dark | HSL lightness + 8 个百分点 | clamp 上限 70；保护回调 ≥ 4.5:1 |
| Light | HSL lightness − 8 个百分点 | clamp 下限 30；保护回调 ≥ 4.5:1 |
| High Contrast | 不派生（返回无效色 → 用 windowBg） | 边框由 QSS 强制 1px `@border@` |

**不变式**（由单测保证，SC-001/FR-003）：
- Dark/Light：`|L(dialogBg) − L(windowBg)| ≥ 8`（HSL lightness 百分点）为默认目标；对比度保护触发时可降级但须 ≥ 2pt（solarized-dark 即此例）
- 全部族：dialogBg 与 text 对比度 ≥ 4.5:1（WCAG AA，FR-003 硬约束，优先于亮度差）
- 色相/饱和度与 windowBg 保持一致（同一配色家族）

## 3. 弹窗类型清单（全覆盖契约）

| # | 弹窗 | 组件 | 状态 | 触发入口 |
|---|---|---|---|---|
| 1 | 帮助 | `FramelessDialog` | 改造前已无边框 | 菜单 Help |
| 2 | 关于 | `FramelessDialog` | 同上 | 菜单 About |
| 3 | 打开数据文件 | `ThemedFileDialog` | 同上；内嵌 `QFileDialog` 由 `buildFileDialogQss` 显式注入主题样式（Qt 5.15 内部样式表屏蔽应用 QSS） | Ctrl+O / 菜单 |
| 4 | 导出主窗口图片 | `ThemedFileDialog` | 同上 | 菜单/工具栏 |
| 5 | 导出 Python 命令 | `ThemedFileDialog` | 同上 | 菜单 |
| 6 | 设置日志路径 | `ThemedFileDialog` | 同上 | 日志菜单 |
| 7 | 布局设置 | `LayoutSettingsDialog` | 同上 | 菜单/子窗口 |
| 8 | 警告（导出失败等） | `ThemedMessageBox`（新） | **改造前系统标题栏** | MainWindow × 2 |
| 9 | 确认（清除日志） | `ThemedMessageBox`（新） | **改造前系统标题栏** | 日志菜单 |
| 10 | 警告（日志操作失败） | `ThemedMessageBox`（新） | **改造前系统标题栏** | LogSettingsController × 2 |

> 契约：1–7 为"背景层自动生效"（零代码改动，随 QSS）；8–10 为"调用点替换"（5 处 QMessageBox → showThemedMessageBox）。

## 4. ThemedMessageBox 组件模型

```
ThemedMessageBox (QDialog, Qt::Dialog | Qt::FramelessWindowHint)
├── titleBar_      ← buildDialogTitleBar(owner, title)  [共享标题栏工厂]
└── body
    ├── iconLabel_ ← QStyle 标准图标（随 palette，状态语义：Info/Warning/Critical/Question）
    ├── textLabel_ ← 消息正文（自动换行，wordWrap）
    └── buttonRow_ ← 按 StandardButtons 生成 QPushButton 行（主按钮 = primary QSS）
```

**工厂 API**：

```cpp
QMessageBox::StandardButton showThemedMessageBox(
    QWidget* parent,
    QMessageBox::Icon icon,      // Information / Warning / Critical / Question
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons = QMessageBox::Ok,   // 支持 Ok / Yes | No / Ok | Cancel 等组合
    QMessageBox::StandardButton defaultButton = QMessageBox::Ok);
```

**语义契约**：
- 模态执行（exec），返回用户点击的按钮（`QMessageBox::StandardButton` 值），与 QMessageBox 静态方法行为等价
- 无用户输入时（直接关闭/点 X），返回与默认按钮一致的语义（按 defaultButton）
- 图标仅为视觉语义（不改变返回逻辑）；按钮文字沿用 Qt 标准映射（Yes/No/OK/Cancel）
- 复用主题 QSS：背景 @dialogBg@、标题栏 @panelBg@、按钮 @controlBg@/primary accent

## 5. 状态变更（弹窗生命周期）

每个弹窗统一状态机（改造前后一致，行为不回归）：

```
closed → shown(exec 模态) → { userActed → closed, closed(关闭按钮/X/Esc) }
```

- 关闭按钮 / Esc / X：关闭弹窗，返回默认按钮语义
- 标题栏拖拽：mousePress/Move/Release 三事件处理，拖动偏移不变
- 主题热切换：QSS 重渲染即时生效，已打开弹窗背景/标题栏/按钮同步更新（FR-002 / SC-002）

## 6. 布局配置修订（WS2，2026-08-30 澄清）

> 目标行为记录于此；004 data-model 的 LayoutConfiguration 语义同步修订见 tasks 阶段。`LayoutConfig` 结构（`src/ui/subwindow/layout_manager.h`）现状字段齐全，本次仅修订**取值语义**与弹窗控件。

### 6.1 字段语义（修订后）

| 字段 | 修订前（004） | 修订后（008 FR-008~010/015） |
|---|---|---|
| `mode` | Row=比例网格行优先 / Column=比例网格列优先 / Grid=默认行优先 | **Row=一列多行 N×1 / Column=一行多列 1×N / Grid=比例网格+优先级** |
| `gridDirection` | 仅 Grid 生效，无约束时定比例方向 | **仅 Grid 生效**；决定**填充方向**（行优先 = 按行填充、列优先 = 按列填充，2026-08-30 再修订）与**约束轴**（FR-010；二次修订：约束轴 = 生效轴——行优先 → 最大列数、列优先 → 最大行数） |
| `maxRows` | 0=未设置；任何模式下约束优先 | **仅 Grid+Column 优先生效**（行数受限、列自适应）；其余模式忽略但值保留 |
| `maxCols` | 同上 | **仅 Grid+Row 优先生效**（列数受限、行自适应）；其余模式忽略但值保留 |
| `sameSize` | 不变 | 不变 |
| `spacing` | 常量 4、不对用户暴露 | **可配置**：0–50 px，默认 6（以代码现状为准）；三种模式均生效 |

### 6.2 computeGrid 语义（目标）

```
mode == Row      → {rows=n, cols=1}   // 一列多行 N×1（每个子窗口占一行）
mode == Column   → {rows=1, cols=n}   // 一行多列 1×N（每个子窗口占一列）
mode == Grid:
  direction == Row:    maxCols>0 → cols=min(n,maxCols), rows=ceil(n/cols)
                       否则       → cols=round(√n),     rows=ceil(n/cols)
  direction == Column: maxRows>0 → rows=min(n,maxRows), cols=ceil(n/rows)
                       否则       → rows=round(√n),     cols=ceil(n/rows)
windowCount ≤ 0 → 空网格 {0,0}
```

**填充方向**（2026-08-30 再修订）：`gridDirection` 同时决定填充顺序——行优先按行填充（index 从左到右推进，第一行填满再下一行）、列优先按列填充（index 从上到下推进，第一列填满再下一列）；例：7 个子窗口行优先 → 3×3 按行 3+3+1、列优先 → 3×3 按列 3+3+1。`cellPos` 为单一事实源（cellRect / lastCellSpan / 容器 relayout 共用）。

**不变式**（单测保证，SC-006）：
- Row 模式任何配置下均为 1 列（一列多行）；Column 模式任何配置下均为 1 行（一行多列）（FR-008）
- Grid 模式仅沿生效轴读取约束（行优先 → maxCols、列优先 → maxRows，2026-08-30 二次修订），另一轴约束值保留但不参与计算（FR-010 / Edge Case）
- 容量不足时（n > 轴约束容量）另一维继续增长，全部子窗口可见可访问（FR-014 语义移入 Grid）

### 6.3 约束轴辅助（单一事实源）

```
ConstraintAxis constraintAxis(LayoutConfig):   // 生效约束轴（2026-08-30 二次修订）
  mode ∈ {Row, Column}              → None
  mode == Grid && direction == Row    → Column   // 行优先 → 最大列数生效
  mode == Grid && direction == Column → Row      // 列优先 → 最大行数生效
```

弹窗控件可用性矩阵（`LayoutSettingsDialog`）与 `computeGrid` 约束分支共用此判定：

| 模式 | 优先级 radio | 最大行数 | 最大列数 |
|---|---|---|---|
| By Row | 禁用 | 禁用 | 禁用 |
| By Column | 禁用 | 禁用 | 禁用 |
| Grid + 行优先 | 启用（互斥单选） | 灰显 | 启用（生效） |
| Grid + 列优先 | 启用（互斥单选） | 启用（生效） | 灰显 |

所有控件始终可见（不切换 `setVisible`），仅切换 `setEnabled`——避免模式/优先级切换造成弹窗几何跳动（SC-006）。

### 6.4 默认配置（恢复默认目标，FR-014）

`mode=Grid, gridDirection=Row, maxRows=0, maxCols=0, sameSize=false, spacing=6`

### 6.5 实时排列预览（FR-013）

- 组件：`LayoutPreviewWidget`（自绘），输入 = 可见子窗口数 N + 当前 `LayoutConfig`
- 几何：`computeGrid` + `cellRect`（复用已测纯函数），cell 色块 `@panelBg@` + 1px `@border@`，背景 `@viewBg@`
- 空态：N=0 显示空提示；N=1 显示单格铺满（Edge Cases）
- 计数来源：`SubwindowContainer::visibleSubwindowCount()`，由 `MainWindow` 在打开/子窗口数变化时同步


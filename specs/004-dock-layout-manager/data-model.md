# 数据模型: 子窗口布局管理

> Phase 1 输出。对应 spec FR-001~019；描述实体、关系、布局计算规则与验证规则。

## 1. 实体

### Subwindow（渲染子窗口）

| 字段 | 类型 | 说明 |
|------|------|------|
| id | string（唯一） | 创建时生成，用于命令返回、日志与选中标识 |
| title | string | 显示于标题区；`create_window("曲线图")` 传入 |
| view | QWidget（占位渲染视图） | 子窗口主体内容；render 层未实装前为占位 widget（FR-003） |
| displayState | enum { Normal, Maximized, Fullscreen } | 显示状态（spec Key Entities） |
| selected | bool | 单击选中；作为最大化/全屏作用对象（spec Assumptions） |
| sizeBeforeSameSize | QSize（可选） | 关闭"保持相同宽高"时恢复的独立尺寸（FR-010） |
| viewState | opaque | 缩放/视角/选中数据等；由后续渲染功能定义，本次占位保存/恢复（FR-019） |

生命周期：创建（PyShell 命令 / 菜单栏，均经命令行执行，命令文本回显、返回值打印，FR-027）→ 参与排列 →（可选）最大化/全屏 → 退出恢复 → 关闭。子窗口在排列切换、最大化/全屏进出时**不销毁**（hide/show 或 re-parent），保证 FR-013 / FR-019。

### LayoutConfiguration（布局配置）

| 字段 | 类型 | 默认 | 校验 |
|------|------|------|------|
| mode | enum { Row, Column, Grid } | Grid | — |
| gridDirection | enum { Row, Column } | Row | 填充方向 + 约束轴（FR-010，2026-08-30 008 修订 / 二次修订）：仅 mode=Grid 时生效；行优先 = 按行填充（第一行从左到右填满再下一行）、列优先 = 按列填充（第一列从上到下填满再下一列）；同时是"仅生效轴约束参与计算"的单一判定源（ConstraintAxis）——行优先 → 最大列数、列优先 → 最大行数 |
| maxRows | int | 0（未设置） | 1–10 整数；非法输入回退上次有效值（FR-012）；仅 网格+列优先 参与计算（行数受限、列自适应），其他模式值保留但忽略（008 修订 / 二次修订） |
| maxCols | int | 0（未设置） | 同上；仅 网格+行优先 参与计算（列数受限、行自适应），其他模式值保留但忽略（008 修订 / 二次修订） |
| sameSize | bool | false | — |
| spacing | int | 6 | 间隙宽度（FR-015，008 修订）：0–50 px 可配置，默认 6；布局设置界面暴露（QSpinBox） |

### DisplayState（显示状态）

enum { Normal, Maximized, Fullscreen }，附着于 `Subwindow.displayState`（spec Key Entities）。

## 2. 关系

- SubwindowContainer 1 ── * Subwindow（子窗口从属于中央区域容器）
- SubwindowContainer 1 ── 1 LayoutConfiguration（单一全局布局配置，作用于全部已打开子窗口）
- DisplayState 1 ── 1 Subwindow.displayState

## 3. 布局计算规则（LayoutManager）

输入：`n` = 子窗口数，`cfg` = LayoutConfiguration，`available` = 可用尺寸。

- **按行排（FR-004，008 修订）**：一列多行 N×1（每个子窗口占一行），忽略全部约束/优先级（值保留但不参与计算）。示例：1→1×1、2→2×1、3→3×1、5→5×1。
- **按列排（FR-005，008 修订）**：一行多列 1×N（每个子窗口占一列），忽略全部约束/优先级。示例：1→1×1、2→1×2、3→1×3、5→1×5。
- **网格（无约束，FR-006）**：行优先（默认）比例网格：cols = round(√n)，rows = ceil(n / cols)。示例：1→1×1、2→2×1、3→2×2、5→3×2、12→4×3、13→4×4。列优先 = 转置：rows = round(√n)，cols = ceil(n / rows)。示例：1→1×1、2→1×2、3→2×2、5→2×3、12→3×4、13→4×4。填充方向由 `gridDirection` 决定（2026-08-30 008 再修订）：行优先按行填充（第一行从左到右填满再下一行）、列优先按列填充（第一列从上到下填满再下一列）；例：7 个子窗口行优先 → 3×3 按行 3+3+1、列优先 → 3×3 按列 3+3+1。
- **约束轴（FR-007/008/010/014，008 修订 / 2026-08-30 二次修订）**：约束轴 = 生效轴（`constraintAxis(cfg)` 单一判定源，弹窗显隐与计算共用）：
  - 网格 + 行优先（ConstraintAxis::Column）：仅 maxCols > 0 时 cols = min(n, maxCols)，rows = ceil(n / cols)（列数受限、行自适应；每行不超过 maxCols）
  - 网格 + 列优先（ConstraintAxis::Row）：仅 maxRows > 0 时 rows = min(n, maxRows)，cols = ceil(n / rows)（行数受限、列自适应；每列不超过 maxRows）
  - 另一轴约束值保留但不参与计算；容量不足时未受限轴继续增长，全部子窗口可见可访问（FR-014；容器滚动兜底）
- **相同宽高**（FR-009）：cell 尺寸 = 可用区域减去间隙后按网格均分，所有子窗口统一尺寸；关闭后恢复 `sizeBeforeSameSize`（FR-010）。
- **间隙**（FR-015，008 修订）：相邻 cell 间距 = spacing（默认 6，可配置 0–50；SC-009 一致且 ≥4px 的量化在默认与用户配置下均适用）；单子窗口时铺满可用范围、无多余内边距。
- **状态迁移**：Normal ⇄ Maximized（容器内独占，其余子窗口隐藏）；Normal/Maximized ⇄ Fullscreen（主窗口级覆盖，面板临时隐藏）。切换不改变子窗口内容与 viewState（FR-019）。

## 4. 验证规则汇总（来自 spec）

- 非法约束输入（0 / 负数 / 非数字）：明确提示 + 恢复上次有效值（FR-012）
- 容量不足：不丢失、不遮挡、可访问（FR-014）
- 相同宽高开/关：统一尺寸（差异 <1px，SC-004）/ 恢复独立尺寸（FR-010）
- 最大化/全屏进出：内容与视图状态不丢失（FR-019），面板显隐完整恢复（FR-017）
- 深/浅主题下布局设置界面清晰可读（US5 场景 3）

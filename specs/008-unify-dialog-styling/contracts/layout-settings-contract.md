# Contract: 布局设置契约（Layout Settings Contract）

**Date**: 2026-08-30 | **Feature**: [../spec.md](../spec.md) | **Data Model**: [../data-model.md](../data-model.md) | **Research**: [../research.md](../research.md)

> 本契约定义布局设置弹窗与布局计算对外承诺的**不变语义**（2026-08-30 澄清修订）；实现必须满足，验收按「Success Criteria 映射」逐项核对。修订 004 语义的部分，004 spec/data-model 同步更新后以本契约为准。

## 1. 排列模式语义契约（`LayoutManager::computeGrid`）

输入：`windowCount`（可见子窗口数 n）+ `LayoutConfig`。输出 `GridLayout{rows, cols}`。

| 模式 | 规则 | 示例（n） |
|---|---|---|
| `Row`（按行排） | 一列多行：`{n, 1}`；忽略 maxRows/maxCols/gridDirection | n=1→1×1, 2→2×1, 5→5×1 |
| `Column`（按列排） | 一行多列：`{1, n}`；同上 | n=1→1×1, 2→1×2, 5→1×5 |
| `Grid` + Row 优先 | maxRows>0 → `rows=min(n,maxRows)`、`cols=ceil(n/rows)`；否则比例 `cols=round(√n)`、`rows=ceil(n/cols)` | 无约束 5→3×2；maxRows=2, n=5→2×3 |
| `Grid` + Column 优先 | maxCols>0 → `cols=min(n,maxCols)`、`rows=ceil(n/cols)`；否则比例 `rows=round(√n)`、`cols=ceil(n/rows)` | 无约束 5→2×3；maxCols=2, n=5→3×2 |
| n ≤ 0 | 空网格 `{0, 0}`，不崩溃 | — |

**不变式**：
- Row/Column 模式**不受任何约束/优先级影响**（FR-008）
- Grid 模式仅沿优先级轴读取约束；另一轴约束值**保留但不参与计算**（FR-009/010）
- 容量不足时（n 超出轴约束容量）另一维继续增长，全部子窗口可见可访问（FR-014 语义移入 Grid）
- `lastCellSpan` / `cellSize` / `cellRect` 公式不变（`spacing` 已由 `cfg.spacing` 贯通）

## 2. 约束轴契约（`ConstraintAxis`）

```cpp
// ui/subwindow/layout_manager.h
enum class ConstraintAxis { None, Row, Column };
ConstraintAxis LayoutManager::constraintAxis(const LayoutConfig& cfg) const;
```

| cfg | 返回 |
|---|---|
| mode ∈ {Row, Column} | `None` |
| mode == Grid && gridDirection == Row | `Row` |
| mode == Grid && gridDirection == Column | `Column` |

用途：弹窗控件显隐（§3）与 `computeGrid` 约束分支的**单一判定源**。

## 3. 弹窗控件显隐矩阵契约（`LayoutSettingsDialog`）

| 控件 | By Row | By Column | Grid + Row 优先 | Grid + Column 优先 |
|---|---|---|---|---|
| 排列模式分段按钮组（Grid/By Row/By Column） | 显示 | 显示 | 显示 | 显示 |
| 优先级 radio（By row / By column） | **隐藏** | **隐藏** | 显示 | 显示 |
| 最大行数 spinbox | **隐藏** | **隐藏** | **显示** | **隐藏** |
| 最大列数 spinbox | **隐藏** | **隐藏** | **隐藏** | **显示** |
| 间隙宽度 spinbox（0–50，默认 6） | 显示 | 显示 | 显示 | 显示 |
| 保持相同宽高 checkbox | 显示 | 显示 | 显示 | 显示 |
| 恢复默认按钮 | 显示 | 显示 | 显示 | 显示 |

**规则**：
- 模式切换**即时生效**（点击分段按钮即发射 `configChanged`，无需确认，FR-011 精神）
- 隐藏 spinbox 的当前值保留在 `LayoutConfig` 中，但按 §1 不参与计算（Edge Case）
- 显隐切换不得造成弹窗高度跳动（SC-006）：预留行高度或布局策略保证

## 4. 恢复默认契约（FR-014）

点击「恢复默认」后，弹窗全部控件与发射的 `configChanged` 配置 = 默认值：

```
mode=Grid, gridDirection=Row, maxRows=0, maxCols=0, sameSize=false, spacing=6
```

- 仅重置布局配置，不影响子窗口内容与视图状态（Edge Case）

## 5. 间隙宽度契约（FR-015）

- 控件：spinbox，范围 **0–50**，默认 **6**，单位 px
- 生效：全部三种模式的相邻子窗口间距（`SubwindowContainer::relayout` 已 `grid_->setSpacing(cfg_.spacing)`；`cellSize`/`cellRect` 公式已扣除 `(cols-1)*spacing` / `(rows-1)*spacing`）
- 边界：间隙 0 → 子窗口无缝相邻（边框仍分隔）；间隙 50 → 子窗口仍完整可见、不溢出（Edge Case）
- 修订 004：spacing 由常量 4 改为可配置，默认以代码现状 6 为准

## 6. 实时排列预览契约（FR-013）

- 组件：`LayoutPreviewWidget`（QWidget 自绘），显示于弹窗内
- 输入：`setPreviewCount(n)`（可见子窗口数，来源 `SubwindowContainer::visibleSubwindowCount()`）+ 当前 `LayoutConfig`
- 几何：`computeGrid` + `cellRect`（复用已测纯函数，预览与真实排布**必然一致**）
- 刷新：任一配置变更或计数变化即重绘
- 空态：n=0 显示空状态提示；n=1 显示单格铺满（Edge Case）

## 7. Success Criteria 映射

| SC | 验收方式 | 对应契约项 |
|---|---|---|
| SC-006 三模式排布互不相同且符合定义 | 自动：`ctest -R layout_manager`（修订后）；手动：3 子窗口切换截图 | §1 / §2 / §3 |
| SC-007 四项界面能力 + 预览与真实一致 + 恢复默认 | 自动：`constraintAxis` 单测；手动：弹窗操作矩阵 | §3 / §4 / §5 / §6 |

## 8. 范围外（Out of Scope）

- 布局配置持久化（QSettings）：004 范围未含，本次不新增
- Python 命令层对布局的接口：004 澄清明确不提供
- 全屏/最大化路径的预览联动：最大化期间预览按可见计数绘制（沿用容器 `visibleSubwindowCount` 语义）

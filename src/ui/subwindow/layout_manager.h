// ===== 子窗口布局计算（004-dock-layout-manager / 008-unify-dialog-styling WS2）=====
// 纯布局逻辑：输入子窗口数/布局配置/可用尺寸 -> 输出网格行列数与各 cell 几何。
// 不依赖任何 QWidget，仅用 QtCore 类型，可在无 GUI 环境单测（tests/cpp/layout_manager_test.cpp）。
// 语义对应 specs/004-dock-layout-manager/data-model.md 第 3 节（2026-08-30 008 修订）。
#pragma once

#include <QRect>
#include <QSize>
#include <utility>

namespace perception {
namespace ui {

// 排列模式（FR-004~006；2026-08-30 008 澄清修订语义）：
//   Row = 按行排：一列多行（N×1，每个子窗口占一行），忽略全部约束/优先级；
//   Column = 按列排：一行多列（1×N，每个子窗口占一列），忽略全部约束/优先级；
//   Grid = 网格：比例网格 + 优先级轴（gridDirection）约束。
enum class LayoutMode { Row, Column, Grid };

// Grid 模式下的填充方向（FR-010；2026-08-30 008 再修订）：
//   Row   = 行优先：按行填充（第一行从左到右填满再下一行，如 7 个 → 3+3+1）；
//   Column= 列优先：按列填充（第一列从上到下填满再下一列，如 7 个 → 3+3+1 纵向）。
// 仅 mode == Grid 时生效。约束轴与填充方向解耦（2026-08-30 二次修订）：
// 行优先 → 仅最大列数生效（每行列数受限、行自适应）；列优先 → 仅最大行数生效。
enum class GridDirection { Row, Column };

// 约束轴 = 生效约束轴（FR-010 / 契约 §2，2026-08-30 二次修订）：
// 弹窗控件显隐与 computeGrid 约束分支的单一判定源。
//   mode ∈ {Row, Column} → None（无约束轴，全部约束不参与计算）；
//   Grid + 行优先 → Column（最大列数生效：列数受限、行自适应）；
//   Grid + 列优先 → Row（最大行数生效：行数受限、列自适应）。
enum class ConstraintAxis { None, Row, Column };

// 布局配置（对应数据模型 LayoutConfiguration）
struct LayoutConfig {
    LayoutMode mode = LayoutMode::Grid;
    // Grid 模式填充方向：Row = 行优先（按行填充），Column = 列优先（按列填充）。
    // 仅 mode == Grid 时读取；Row/Column 模式由自身语义决定（一列多行/一行多列）。
    GridDirection gridDirection = GridDirection::Row;
    // 最大行/列数：0 = 未设置（Unlimited）。
    //   Grid + 行优先（constraintAxis == Column，2026-08-30 二次修订）：仅 maxCols 参与计算——
    //     maxCols>0 → cols = min(n, maxCols)、rows = ceil(n/cols)（列数受限、行自适应，不丢窗口 FR-014）；
    //     maxCols==0 → 比例网格 cols = round(√n)、rows = ceil(n/cols)。
    //   Grid + 列优先（constraintAxis == Row）：仅 maxRows 参与计算——对称。
    //   Row/Column 模式：全部约束忽略但值保留（切换回 Grid 后原值仍生效，Edge Case）。
    int        maxRows = 0;
    int        maxCols = 0;
    bool       sameSize = false;  // 保持相同宽高（FR-009）
    int        spacing = 6;       // 相邻子窗口间隙（px，FR-015 / 008：可配置 0–50，默认 6）
};

// 计算得到的网格结构
struct GridLayout {
    int rows = 0;
    int cols = 0;
};

class LayoutManager {
public:
    // FR-004~008 / FR-010（008 修订语义；2026-08-30 二次修订：约束轴 = 生效轴）：
    //   Row  → {n, 1}；Column → {1, n}（忽略全部约束/优先级，FR-008）。
    //   Grid + 行优先（axis=Column）：maxCols>0 → cols=min(n,maxCols)、rows=ceil(n/cols)；
    //                                  否则     → cols=round(√n)、rows=ceil(n/cols)（比例网格）。
    //   Grid + 列优先（axis=Row）：maxRows>0 → rows=min(n,maxRows)、cols=ceil(n/rows)；
    //                              否则     → rows=round(√n)、cols=ceil(n/rows)（转置比例网格）。
    //   windowCount <= 0 返回空网格。
    GridLayout computeGrid(int windowCount, const LayoutConfig& cfg) const;

    // 约束轴（FR-010 / 契约 §2）：语义见 ConstraintAxis 注释。
    // 弹窗控件显隐矩阵（LayoutSettingsDialog）与 computeGrid 约束分支共用此判定。
    ConstraintAxis constraintAxis(const LayoutConfig& cfg) const;

    // 第 index 个子窗口在网格中的位置（first = 行号，second = 列号）。
    // 填充方向随 gridDirection：Row = 行优先（index 按行从左到右推进），
    // Column = 列优先（index 按列从上到下推进）——cellRect / lastCellSpan / 容器
    // relayout 共用的单一事实源（008 修订：优先级同时决定填充方向）。
    // 越界（index < 0 || index >= rows*cols）返回 {-1, -1}。
    std::pair<int, int> cellPos(int index, const GridLayout& grid,
                                const LayoutConfig& cfg) const;

    // 最后一个可见子窗口的跨度（QSize.width = 列跨度，.height = 行跨度）。
    // 位置按 cellPos（填充方向感知）；sameSize=true：恒为 1x1（各 cell 等大，
    // 末尾剩余 cell 留白）；sameSize=false：最后一个子窗口从其位置延伸至网格
    // 右下角，占满剩余空间。
    QSize lastCellSpan(int windowCount, const GridLayout& grid, const LayoutConfig& cfg) const;

    // FR-009/FR-015：统一 cell 尺寸 = 可用尺寸扣除 (cols-1)*spacing / (rows-1)*spacing 后均分。
    QSize cellSize(const QSize& available, const GridLayout& grid, const LayoutConfig& cfg) const;

    // 第 index 个子窗口的矩形（含间隙偏移，位置按 cellPos 填充方向）；越界返回空矩形。
    QRect cellRect(const QSize& available, int index, const GridLayout& grid,
                   const LayoutConfig& cfg) const;
};

}  // namespace ui
}  // namespace perception

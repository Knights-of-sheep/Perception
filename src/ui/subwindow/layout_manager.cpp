#include "ui/subwindow/layout_manager.h"

#include <algorithm>
#include <cmath>

namespace perception {
namespace ui {

namespace {

// ceil(a / b)，b > 0
int ceilDiv(int a, int b) { return (a + b - 1) / b; }

// 比例网格基准列数：cols = round(√n)（行优先）；列优先时取其行数对称使用
int proportionalBase(int n) {
    return static_cast<int>(std::lround(std::sqrt(static_cast<double>(n))));
}

}  // namespace

ConstraintAxis LayoutManager::constraintAxis(const LayoutConfig& cfg) const {
    // 约束轴 = 生效约束轴（FR-010 / 契约 §2，2026-08-30 二次修订）：
    // 行优先 → 最大列数生效（Column）；列优先 → 最大行数生效（Row）。
    // Row/Column 模式无约束轴；弹窗显隐与 computeGrid 共用此判定。
    if (cfg.mode != LayoutMode::Grid) return ConstraintAxis::None;
    return cfg.gridDirection == GridDirection::Row ? ConstraintAxis::Column
                                                   : ConstraintAxis::Row;
}

GridLayout LayoutManager::computeGrid(int windowCount, const LayoutConfig& cfg) const {
    const int n = windowCount;
    if (n <= 0) return {};  // 空网格，不崩溃（Edge Cases）
    // 按行排 / 按列排：一列多行 / 一行多列，忽略全部约束与优先级（FR-008）
    if (cfg.mode == LayoutMode::Row) return {n, 1};
    if (cfg.mode == LayoutMode::Column) return {1, n};
    // Grid：仅沿生效约束轴读取约束，另一轴约束值保留但不参与计算（FR-010）
    const ConstraintAxis axis = constraintAxis(cfg);
    if (axis == ConstraintAxis::Column) {
        // Grid + 行优先：最大列数生效（列数受限、行自适应）
        if (cfg.maxCols > 0) {
            const int cols = std::min(n, cfg.maxCols);  // 每行不超过 maxCols（FR-014 不丢窗口）
            return {ceilDiv(n, cols), cols};
        }
        const int cols = proportionalBase(n);
        return {ceilDiv(n, cols), cols};  // 行优先比例网格
    }
    // axis == ConstraintAxis::Row（Grid + 列优先）：最大行数生效（行数受限、列自适应）
    if (cfg.maxRows > 0) {
        const int rows = std::min(n, cfg.maxRows);  // 每列不超过 maxRows（FR-014 不丢窗口）
        return {rows, ceilDiv(n, rows)};
    }
    const int rows = proportionalBase(n);
    return {rows, ceilDiv(n, rows)};  // 列优先比例网格（转置）
}

std::pair<int, int> LayoutManager::cellPos(int index, const GridLayout& grid,
                                           const LayoutConfig& cfg) const {
    if (index < 0 || grid.rows <= 0 || grid.cols <= 0 ||
        index >= grid.rows * grid.cols) {
        return {-1, -1};  // 越界（调用方防御；computeGrid 保证 n ≤ rows*cols）
    }
    // 填充方向 = 优先级方向（008 修订）：行优先按行推进、列优先按列推进
    if (cfg.gridDirection == GridDirection::Column) {
        return {index % grid.rows, index / grid.rows};  // (row, col)，先填满列
    }
    return {index / grid.cols, index % grid.cols};  // (row, col)，先填满行
}

QSize LayoutManager::lastCellSpan(int windowCount, const GridLayout& grid,
                                  const LayoutConfig& cfg) const {
    const int n = windowCount;
    if (n <= 0 || grid.rows <= 0 || grid.cols <= 0) return QSize(1, 1);
    // 保持相同宽高：各 cell 等大，末尾剩余 cell 留白（最后一个不延伸）
    if (cfg.sameSize) return QSize(1, 1);
    // 否则最后一个子窗口从自身位置延伸至网格右下角，占满剩余空间
    const auto pos = cellPos(n - 1, grid, cfg);
    if (pos.first < 0) return QSize(1, 1);
    return QSize(grid.cols - pos.second, grid.rows - pos.first);  // (列跨度, 行跨度)
}

QSize LayoutManager::cellSize(const QSize& available, const GridLayout& grid,
                              const LayoutConfig& cfg) const {
    if (grid.rows <= 0 || grid.cols <= 0) return {};
    const int w = (available.width() - (grid.cols - 1) * cfg.spacing) / grid.cols;
    const int h = (available.height() - (grid.rows - 1) * cfg.spacing) / grid.rows;
    return QSize(std::max(0, w), std::max(0, h));
}

QRect LayoutManager::cellRect(const QSize& available, int index, const GridLayout& grid,
                              const LayoutConfig& cfg) const {
    if (grid.rows <= 0 || grid.cols <= 0 || index < 0 ||
        index >= grid.rows * grid.cols) {
        return {};
    }
    const auto pos = cellPos(index, grid, cfg);  // 填充方向感知（行/列优先）
    const QSize cs = cellSize(available, grid, cfg);
    return QRect(pos.second * (cs.width() + cfg.spacing),
                 pos.first * (cs.height() + cfg.spacing), cs.width(), cs.height());
}

}  // namespace ui
}  // namespace perception

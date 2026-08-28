#include "ui/subwindow/layout_manager.h"

#include <algorithm>
#include <cmath>

namespace perception {
namespace ui {

namespace {

// ceil(a / b)，b > 0
int ceilDiv(int a, int b) { return (a + b - 1) / b; }

}  // namespace

GridLayout LayoutManager::computeGrid(int windowCount, const LayoutConfig& cfg) const {
    const int n = windowCount;
    if (n <= 0) return {};
    // 约束优先（FR-007/008/014）：任何模式下，设置了最大列/行数即按约束排列。
    //   仅 maxCols：列数 ≤ maxCols 固定，行数 = ceil(n/cols)（行自适应，不丢窗口）。
    //   仅 maxRows：行数 ≤ maxRows 固定，列数 = ceil(n/rows)。
    if (cfg.maxCols > 0) {
        const int cols = std::min(n, cfg.maxCols);
        return {ceilDiv(n, cols), cols};
    }
    if (cfg.maxRows > 0) {
        const int rows = std::min(n, cfg.maxRows);
        return {rows, ceilDiv(n, rows)};
    }
    // 无约束：保持比例的网格（不再强制方阵/单行/单列）。
    //   行优先（优先行排 / 网格默认方向）：cols 取最接近 √n 的整数（round），
    //     rows = ceil(n/cols)，使子窗口尽量接近 1:1，但允许 2:1、3:2、4:3 等比例
    //     （1→1×1, 2→2×1, 3→2×2, 5→3×2, 12→4×3, 13→4×4）。
    //   列优先（优先列排 / Grid + 优先列）：转置
    //     （1→1×1, 2→1×2, 3→2×2, 5→2×3, 12→3×4, 13→4×4）。
    const int nearSqrt =
        static_cast<int>(std::lround(std::sqrt(static_cast<double>(n))));
    const bool columnPreferred =
        cfg.mode == LayoutMode::Column ||
        (cfg.mode == LayoutMode::Grid && cfg.gridDirection == GridDirection::Column);
    if (columnPreferred) {
        const int rows = nearSqrt;
        return {rows, ceilDiv(n, rows)};
    }
    const int cols = nearSqrt;
    return {ceilDiv(n, cols), cols};
}

QSize LayoutManager::lastCellSpan(int windowCount, const GridLayout& grid,
                                  const LayoutConfig& cfg) const {
    const int n = windowCount;
    if (n <= 0 || grid.rows <= 0 || grid.cols <= 0) return QSize(1, 1);
    // 保持相同宽高：各 cell 等大，末尾剩余 cell 留白（最后一个不延伸）
    if (cfg.sameSize) return QSize(1, 1);
    // 否则最后一个子窗口从自身位置延伸至网格右下角，占满剩余空间
    const int last = n - 1;
    const int row = last / grid.cols;
    const int col = last % grid.cols;
    return QSize(grid.cols - col, grid.rows - row);  // (列跨度, 行跨度)
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
    const QSize cs = cellSize(available, grid, cfg);
    const int col = index % grid.cols;
    const int row = index / grid.cols;
    return QRect(col * (cs.width() + cfg.spacing), row * (cs.height() + cfg.spacing),
                 cs.width(), cs.height());
}

}  // namespace ui
}  // namespace perception

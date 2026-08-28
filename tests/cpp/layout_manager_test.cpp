// ===== 子窗口布局计算单测（004-dock-layout-manager）=====
// 覆盖 FR-004~009 / FR-014 / FR-015 与 SC-003/004/009 的计算语义。
// 仅依赖 QtCore 类型（QSize/QRect），无 GUI 平台依赖，红-绿 TDD。
#include "ui/subwindow/layout_manager.h"

#include <cassert>

using perception::ui::GridDirection;
using perception::ui::GridLayout;
using perception::ui::LayoutConfig;
using perception::ui::LayoutManager;
using perception::ui::LayoutMode;

namespace {

// 断言失败输出上下文后中断（测试可执行文件断言保护见 tests/cpp/CMakeLists.txt）
void expectGrid(int rows, int cols, int n, const LayoutConfig& cfg) {
    const auto g = LayoutManager().computeGrid(n, cfg);
    if (g.rows != rows || g.cols != cols) {
        fprintf(stderr, "computeGrid(n=%d) expected (%d,%d) got (%d,%d)\n", n, rows, cols,
                g.rows, g.cols);
        assert(false);
    }
}

void expectSize(int w, int h, const QSize& available, const GridLayout& g,
                const LayoutConfig& cfg) {
    const auto s = LayoutManager().cellSize(available, g, cfg);
    if (s.width() != w || s.height() != h) {
        fprintf(stderr, "cellSize expected (%d,%d) got (%d,%d)\n", w, h, s.width(), s.height());
        assert(false);
    }
}

// 断言最后一个可见子窗口跨度（QSize(w=列跨度, h=行跨度)）
void expectSpan(int w, int h, int n, const GridLayout& g, const LayoutConfig& cfg) {
    const auto s = LayoutManager().lastCellSpan(n, g, cfg);
    if (s.width() != w || s.height() != h) {
        fprintf(stderr, "lastCellSpan(n=%d) expected (%d,%d) got (%d,%d)\n", n, w, h,
                s.width(), s.height());
        assert(false);
    }
}

}  // namespace

int main() {
    LayoutManager lm;

    // ---- FR-004：优先行排（By Row）：无约束 = 保持比例网格（优先行，非方阵）----
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Row;
        expectGrid(1, 1, 1, cfg);
        expectGrid(2, 1, 2, cfg);   // 2 → 2×1
        expectGrid(2, 2, 3, cfg);   // 3 → 2×2
        expectGrid(3, 2, 5, cfg);   // 5 → 3×2
        expectGrid(4, 3, 12, cfg);  // 12 → 4×3
        expectGrid(4, 4, 13, cfg);  // 13 → 4×4
        cfg.maxRows = 2;
        expectGrid(2, 3, 5, cfg);  // 最多 2 行 -> 3 列
        cfg.maxRows = 5;
        expectGrid(3, 1, 3, cfg);  // n < maxRows -> 行数=n，每行一个
    }
    // ---- FR-005：优先列排（By Column）：无约束 = 保持比例网格（优先列，转置）----
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Column;
        expectGrid(1, 1, 1, cfg);
        expectGrid(1, 2, 2, cfg);   // 2 → 1×2
        expectGrid(2, 2, 3, cfg);   // 3 → 2×2
        expectGrid(2, 3, 5, cfg);   // 5 → 2×3
        expectGrid(3, 4, 12, cfg);  // 12 → 3×4
        expectGrid(4, 4, 13, cfg);  // 13 → 4×4
        cfg.maxCols = 2;
        expectGrid(3, 2, 5, cfg);  // 最多 2 列 -> 3 行
    }
    // ---- FR-006：网格（默认无约束 = 行优先比例网格，不再强制方阵）----
    {
        LayoutConfig cfg;  // mode = Grid
        expectGrid(1, 1, 1, cfg);
        expectGrid(2, 1, 2, cfg);   // 2 → 2×1
        expectGrid(2, 2, 3, cfg);   // 3 → 2×2
        expectGrid(2, 2, 4, cfg);   // 4 → 2×2
        expectGrid(3, 2, 5, cfg);   // 5 → 3×2
        expectGrid(3, 3, 9, cfg);   // 9 → 3×3（恰为方阵）
        expectGrid(4, 3, 12, cfg);  // 12 → 4×3
        expectGrid(4, 4, 13, cfg);  // 13 → 4×4
    }
    // ---- 2026-08-29 反馈：Grid + 优先列（gridDirection=Column）无约束转置（同 FR-005）----
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Grid;
        cfg.gridDirection = GridDirection::Column;
        expectGrid(1, 1, 1, cfg);
        expectGrid(1, 2, 2, cfg);   // 2 → 1×2
        expectGrid(2, 2, 3, cfg);   // 3 → 2×2
        expectGrid(2, 3, 5, cfg);   // 5 → 2×3
        expectGrid(3, 4, 12, cfg);  // 12 → 3×4
        expectGrid(4, 4, 13, cfg);  // 13 → 4×4
        cfg.maxCols = 2;            // 约束仍优先
        expectGrid(3, 2, 5, cfg);
        cfg.maxCols = 0;
        cfg.maxRows = 2;
        expectGrid(2, 3, 5, cfg);   // 行约束优先
    }
    // ---- Grid + 默认方向（Row）保持行优先（FR-006 无约束语义不变）----
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Grid;
        cfg.gridDirection = GridDirection::Row;
        expectGrid(2, 1, 2, cfg);  // 2 → 2×1
        expectGrid(3, 2, 5, cfg);  // 5 → 3×2
    }
    // ---- FR-007：网格 + 仅最大列数：列数受限，行自适应换行 ----
    {
        LayoutConfig cfg;
        cfg.maxCols = 2;
        expectGrid(3, 2, 5, cfg);
        expectGrid(2, 2, 4, cfg);
        expectGrid(1, 1, 1, cfg);  // n < maxCols 时 cols=n
    }
    // ---- 网格 + 仅最大行数：行数受限，列自适应 ----
    {
        LayoutConfig cfg;
        cfg.maxRows = 2;
        expectGrid(2, 3, 5, cfg);
        expectGrid(2, 2, 4, cfg);
        expectGrid(1, 1, 1, cfg);
    }
    // ---- FR-008 + FR-014：最大行数换列；容量不足时列数保持、行数继续增长，全部可访问 ----
    {
        LayoutConfig cfg;
        cfg.maxRows = 2;
        cfg.maxCols = 2;
        expectGrid(2, 2, 4, cfg);   // 恰好填满容量
        expectGrid(3, 2, 5, cfg);   // 容量不足：第 5 个排第三行，可见可访问
        expectGrid(5, 2, 10, cfg);  // 10 个 → 5 行 2 列
    }
    {
        LayoutConfig cfg;
        cfg.maxRows = 1;
        cfg.maxCols = 1;
        expectGrid(3, 1, 3, cfg);  // 容量 1：行数继续增长（FR-014）
    }
    // ---- maxRows 单独生效（列数不限时一行即满足）----
    {
        LayoutConfig cfg;
        cfg.maxRows = 2;
        cfg.maxCols = 3;
        expectGrid(2, 3, 6, cfg);
        expectGrid(2, 3, 5, cfg);  // ceil(5/3)=2 行
    }
    // ---- Edge Cases：0 个子窗口返回空网格，不崩溃 ----
    expectGrid(0, 0, 0, LayoutConfig());

    // ---- FR-015 + SC-009：间隙一致 ≥4px（cellSize 扣除间距后均分）----
    {
        LayoutConfig cfg;
        cfg.spacing = 4;
        GridLayout g{1, 2};
        expectSize(48, 100, QSize(100, 100), g, cfg);  // (100-4)/2=48
        GridLayout g2{2, 2};
        expectSize(48, 48, QSize(100, 100), g2, cfg);
    }
    {
        LayoutConfig cfg;
        cfg.spacing = 10;
        GridLayout g{3, 1};
        expectSize(100, 26, QSize(100, 100), g, cfg);  // (100-20)/3=26
    }
    // ---- 单子窗口：铺满可用范围，无间距扣除（Edge Cases）----
    {
        LayoutConfig cfg;
        GridLayout g{1, 1};
        expectSize(100, 100, QSize(100, 100), g, cfg);
    }
    // ---- FR-009 + SC-004：所有 cell 尺寸一致 ----
    {
        LayoutConfig cfg;
        cfg.maxCols = 3;
        const auto g = lm.computeGrid(5, cfg);
        const auto s = lm.cellSize(QSize(640, 480), g, cfg);
        for (int i = 0; i < 5; ++i) {
            const auto r = lm.cellRect(QSize(640, 480), i, g, cfg);
            assert(r.width() == s.width() && r.height() == s.height());
        }
    }
    // ---- cellRect：网格位置（含间隙偏移）----
    {
        LayoutConfig cfg;
        cfg.spacing = 4;
        GridLayout g{2, 2};
        const QSize avail(100, 100);  // cellSize = (48,48)
        assert(lm.cellRect(avail, 0, g, cfg) == QRect(0, 0, 48, 48));
        assert(lm.cellRect(avail, 1, g, cfg) == QRect(52, 0, 48, 48));
        assert(lm.cellRect(avail, 2, g, cfg) == QRect(0, 52, 48, 48));
        assert(lm.cellRect(avail, 3, g, cfg) == QRect(52, 52, 48, 48));
        // 越界 index：返回空矩形，不崩溃
        assert(lm.cellRect(avail, 4, g, cfg).isNull());
    }
    // ---- sameSize 末尾行为：勾选留白 / 未勾选最后一个子窗口占满剩余空间 ----
    {
        // 保持相同宽高：各 cell 等大，末尾剩余 cell 留白（最后一个不延伸）
        LayoutConfig cfg;
        cfg.sameSize = true;
        expectSpan(1, 1, 3, GridLayout{2, 2}, cfg);
        // 未勾选：最后一个子窗口从其位置延伸至网格右下角
        cfg.sameSize = false;
        expectSpan(2, 1, 3, GridLayout{2, 2}, cfg);  // 3 个 2x2：末行仅 1 个，占满末行
        expectSpan(2, 2, 5, GridLayout{3, 3}, cfg);  // 5 个 3x3：从 (1,1) 延伸占满剩余 2x2
        expectSpan(1, 1, 4, GridLayout{2, 2}, cfg);  // 满网格无剩余
        expectSpan(1, 1, 1, GridLayout{1, 1}, cfg);  // 单窗口
        // Row 优先行排 n=5 maxRows=2 -> (2,3)：最后 (1,1) 延伸占满末行剩余列
        LayoutConfig rowCfg;
        rowCfg.mode = LayoutMode::Row;
        rowCfg.maxRows = 2;
        const auto rowGrid = lm.computeGrid(5, rowCfg);
        expectSpan(2, 1, 5, rowGrid, rowCfg);
        // Edge Cases：0 个子窗口
        expectSpan(1, 1, 0, GridLayout{}, cfg);
    }

    puts("layout_manager_test: ALL PASS");
    return 0;
}

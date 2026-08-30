// ===== 子窗口布局计算单测（004-dock-layout-manager / 008-unify-dialog-styling WS2）=====
// 覆盖 FR-004~009 / FR-014 / FR-015 / FR-010（约束轴）与 SC-003/004/006/007/009 的计算语义。
// 2026-08-30 澄清修订（008）：Row=一列多行 N×1、Column=一行多列 1×N、Grid=比例+优先级轴约束；
//   再修订：优先级决定填充方向——行优先按行填充、列优先按列填充（cellPos 单一事实源）；
//   二次修订：约束轴 = 生效轴——行优先 → 最大列数生效、列优先 → 最大行数生效（constraintAxis 单一判定源）。
// 仅依赖 QtCore 类型（QSize/QRect），无 GUI 平台依赖，红-绿 TDD。
#include "ui/subwindow/layout_manager.h"

#include <cassert>
#include <cstdio>
#include <utility>

using perception::ui::ConstraintAxis;
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

// 约束轴判定（FR-010 / 契约 §2）：弹窗显隐与 computeGrid 共用的单一事实源
void expectAxis(ConstraintAxis expected, const LayoutConfig& cfg) {
    const auto a = LayoutManager().constraintAxis(cfg);
    if (a != expected) {
        fprintf(stderr, "constraintAxis expected %d got %d\n", static_cast<int>(expected),
                static_cast<int>(a));
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

    // ========== FR-008（修订）：按行排 = 一列多行 N×1，忽略全部约束/优先级 ==========
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Row;
        expectGrid(1, 1, 1, cfg);
        expectGrid(2, 1, 2, cfg);   // 2 → 2×1（一列多行）
        expectGrid(3, 1, 3, cfg);   // 3 → 3×1
        expectGrid(5, 1, 5, cfg);   // 5 → 5×1（一列多行）
        expectGrid(12, 1, 12, cfg); // 12 → 12×1
        // 约束/优先级对 Row 无影响（值保留但不参与计算，FR-008）
        cfg.maxRows = 2;
        cfg.maxCols = 2;
        cfg.gridDirection = GridDirection::Column;
        expectGrid(5, 1, 5, cfg);
        expectAxis(ConstraintAxis::None, cfg);
    }
    // ========== FR-008（修订）：按列排 = 一行多列 1×N，忽略全部约束/优先级 ==========
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Column;
        expectGrid(1, 1, 1, cfg);
        expectGrid(1, 2, 2, cfg);   // 2 → 1×2（一行多列）
        expectGrid(1, 3, 3, cfg);   // 3 → 1×3
        expectGrid(1, 5, 5, cfg);   // 5 → 1×5（一行多列）
        expectGrid(1, 12, 12, cfg); // 12 → 1×12
        cfg.maxRows = 2;
        cfg.maxCols = 2;
        cfg.gridDirection = GridDirection::Row;
        expectGrid(1, 5, 5, cfg);
        expectAxis(ConstraintAxis::None, cfg);
    }
    // ================= FR-006（修订）：Grid + 行优先（默认）无约束 = 行优先比例网格 =========
    {
        LayoutConfig cfg;  // mode = Grid, gridDirection = Row
        expectGrid(1, 1, 1, cfg);
        expectGrid(2, 1, 2, cfg);   // 2 → 2×1
        expectGrid(2, 2, 3, cfg);   // 3 → 2×2
        expectGrid(2, 2, 4, cfg);   // 4 → 2×2
        expectGrid(3, 2, 5, cfg);   // 5 → 3×2
        expectGrid(3, 3, 9, cfg);   // 9 → 3×3（恰为方阵）
        expectGrid(4, 3, 12, cfg);  // 12 → 4×3
        expectGrid(4, 4, 13, cfg);  // 13 → 4×4
    }
    // ================= FR-006（修订）：Grid + 列优先 无约束 = 转置比例网格 =================
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
    }
    // ============ FR-007/010（二次修订）：Grid + 行优先 → 仅 maxCols 生效 ============
    {
        LayoutConfig cfg;  // Grid + Row 优先
        cfg.maxCols = 2;
        expectGrid(3, 2, 5, cfg);  // 列数受限 2，行自适应（3×2）
        expectGrid(2, 2, 4, cfg);
        expectGrid(1, 1, 1, cfg);  // n < maxCols → cols=n
        cfg.maxCols = 5;
        expectGrid(1, 3, 3, cfg);  // n < maxCols → 一行铺开
        expectAxis(ConstraintAxis::Column, cfg);
        // 另一轴（maxRows）保留但不参与计算：无 maxCols 时 maxRows 被忽略 → 比例网格
        LayoutConfig cfg2;
        cfg2.maxRows = 2;
        expectGrid(3, 2, 5, cfg2);  // 5 → 3×2（比例，非行数固定）
        // 双约束：仅读生效轴 maxCols → 3×2
        LayoutConfig cfg3;
        cfg3.maxRows = 2;
        cfg3.maxCols = 2;
        expectGrid(3, 2, 5, cfg3);
        expectGrid(5, 2, 10, cfg3);  // 容量不足：行继续增长，全部可访问（FR-014）
    }
    // ============ FR-007/010（二次修订）：Grid + 列优先 → 仅 maxRows 生效 ============
    {
        LayoutConfig cfg;
        cfg.mode = LayoutMode::Grid;
        cfg.gridDirection = GridDirection::Column;
        cfg.maxRows = 2;
        expectGrid(2, 3, 5, cfg);  // 行数受限 2，列自适应（2×3）
        expectGrid(2, 2, 4, cfg);
        expectGrid(1, 1, 1, cfg);  // n < maxRows → rows=n
        cfg.maxRows = 5;
        expectGrid(3, 1, 3, cfg);  // n < maxRows → 一列排开
        expectAxis(ConstraintAxis::Row, cfg);
        // 另一轴（maxCols）保留但不参与计算：无 maxRows 时 maxCols 被忽略 → 转置比例
        LayoutConfig cfg2;
        cfg2.mode = LayoutMode::Grid;
        cfg2.gridDirection = GridDirection::Column;
        cfg2.maxCols = 2;
        expectGrid(2, 3, 5, cfg2);  // 5 → 2×3（比例转置，非列数固定）
        // 双约束：仅读生效轴 maxRows → 2×3
        LayoutConfig cfg3;
        cfg3.mode = LayoutMode::Grid;
        cfg3.gridDirection = GridDirection::Column;
        cfg3.maxRows = 2;
        cfg3.maxCols = 2;
        expectGrid(2, 3, 5, cfg3);
        expectGrid(2, 5, 10, cfg3);  // 容量不足：列继续增长（FR-014）
    }
    // ================= 约束轴判定全组合（契约 §2） =================
    {
        expectAxis(ConstraintAxis::Column, LayoutConfig());  // 默认 Grid + Row 优先 → Column（最大列数生效）

        LayoutConfig row; row.mode = LayoutMode::Row;
        expectAxis(ConstraintAxis::None, row);

        LayoutConfig col; col.mode = LayoutMode::Column;
        expectAxis(ConstraintAxis::None, col);

        LayoutConfig gRow; gRow.mode = LayoutMode::Grid; gRow.gridDirection = GridDirection::Row;
        expectAxis(ConstraintAxis::Column, gRow);

        LayoutConfig gCol; gCol.mode = LayoutMode::Grid; gCol.gridDirection = GridDirection::Column;
        expectAxis(ConstraintAxis::Row, gCol);
    }
    // ================= Edge Cases：0 个子窗口返回空网格，不崩溃 =================
    expectGrid(0, 0, 0, LayoutConfig());

    // ---- FR-015 + SC-009：间隙一致 ≥4px（cellSize 扣除间距后均分；spacing 现可配置 0–50）----
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
    {
        // 间隙 0 → 无缝相邻，cell 尺寸 = 均分（边界 FR-015）
        LayoutConfig cfg;
        cfg.spacing = 0;
        GridLayout g{1, 2};
        expectSize(50, 100, QSize(100, 100), g, cfg);
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
        cfg.maxCols = 2;  // Grid + 行优先 → 最大列数生效 → 3×2
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
    // ---- 填充方向（008 再修订）：行优先按行填充、列优先按列填充 ----
    {
        // n=7 无约束：行优先 → 3×3，按行 3+3+1；列优先 → 3×3，按列 3+3+1
        LayoutConfig rowCfg;  // Grid + Row 优先（默认）
        const auto rowGrid = lm.computeGrid(7, rowCfg);
        assert(rowGrid.rows == 3 && rowGrid.cols == 3);
        const std::pair<int, int> rowSeq[7] = {{0, 0}, {0, 1}, {0, 2},
                                               {1, 0}, {1, 1}, {1, 2}, {2, 0}};
        for (int i = 0; i < 7; ++i) assert(lm.cellPos(i, rowGrid, rowCfg) == rowSeq[i]);

        LayoutConfig colCfg;
        colCfg.mode = LayoutMode::Grid;
        colCfg.gridDirection = GridDirection::Column;
        const auto colGrid = lm.computeGrid(7, colCfg);
        assert(colGrid.rows == 3 && colGrid.cols == 3);
        const std::pair<int, int> colSeq[7] = {{0, 0}, {1, 0}, {2, 0},
                                               {0, 1}, {1, 1}, {2, 1}, {0, 2}};
        for (int i = 0; i < 7; ++i) assert(lm.cellPos(i, colGrid, colCfg) == colSeq[i]);
        // 越界：返回 {-1,-1}（index 达到网格容量 rows*cols 即越界）
        assert(lm.cellPos(rowGrid.rows * rowGrid.cols, rowGrid, rowCfg) ==
               std::make_pair(-1, -1));
        assert(lm.cellPos(-1, rowGrid, rowCfg) == std::make_pair(-1, -1));

        // 列优先 cellRect：与行优先互为转置（2×2，cellSize=(48,48)）
        colCfg.spacing = 4;
        GridLayout g2{2, 2};
        const QSize avail2(100, 100);
        assert(lm.cellRect(avail2, 0, g2, colCfg) == QRect(0, 0, 48, 48));
        assert(lm.cellRect(avail2, 1, g2, colCfg) == QRect(0, 52, 48, 48));
        assert(lm.cellRect(avail2, 2, g2, colCfg) == QRect(52, 0, 48, 48));
        assert(lm.cellRect(avail2, 3, g2, colCfg) == QRect(52, 52, 48, 48));
    }
    // ---- 一维模式（Row/Column）：行/列优先映射等价，顺序正确 ----
    {
        LayoutConfig r;
        r.mode = LayoutMode::Row;
        const auto rg = lm.computeGrid(5, r);
        for (int i = 0; i < 5; ++i) assert(lm.cellPos(i, rg, r) == std::make_pair(i, 0));

        LayoutConfig c;
        c.mode = LayoutMode::Column;
        const auto cg = lm.computeGrid(5, c);
        for (int i = 0; i < 5; ++i) assert(lm.cellPos(i, cg, c) == std::make_pair(0, i));
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
        // 列优先 n=7 → 3×3：last=(0,2)，延伸占满剩余 (1 列 × 3 行)
        LayoutConfig colCfg;
        colCfg.mode = LayoutMode::Grid;
        colCfg.gridDirection = GridDirection::Column;
        expectSpan(1, 3, 7, GridLayout{3, 3}, colCfg);
        expectSpan(1, 1, 9, GridLayout{3, 3}, colCfg);  // 满网格无剩余
        // Row 按行排 n=5 → 5×1（一列多行）：最后 (4,0) 无剩余 cell，跨度 1×1
        LayoutConfig rowCfg;
        rowCfg.mode = LayoutMode::Row;
        const auto rowGrid = lm.computeGrid(5, rowCfg);
        expectSpan(1, 1, 5, rowGrid, rowCfg);
        // Edge Cases：0 个子窗口
        expectSpan(1, 1, 0, GridLayout{}, cfg);
    }

    puts("layout_manager_test: ALL PASS");
    return 0;
}

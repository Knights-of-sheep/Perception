// ===== 子窗口布局计算（004-dock-layout-manager）=====
// 纯布局逻辑：输入子窗口数/布局配置/可用尺寸 -> 输出网格行列数与各 cell 几何。
// 不依赖任何 QWidget，仅用 QtCore 类型，可在无 GUI 环境单测（tests/cpp/layout_manager_test.cpp）。
// 语义对应 specs/004-dock-layout-manager/data-model.md 第 3 节。
#pragma once

#include <QRect>
#include <QSize>

namespace perception {
namespace ui {

// 排列模式（FR-004~006；"优先行排/优先列排" 即 Row/Column）
enum class LayoutMode { Row, Column, Grid };

// Grid 模式下的填充方向（2026-08-29 用户反馈：Grid 布局支持设置优先行/优先列）：
// 仅 mode == Grid 时生效，决定无约束比例网格的行优先/列优先。
enum class GridDirection { Row, Column };

// 布局配置（对应数据模型 LayoutConfiguration）
struct LayoutConfig {
    LayoutMode mode = LayoutMode::Grid;
    // Grid 模式填充方向：Row = 行优先（同 FR-004 无约束网格），Column = 列优先
    //（转置，同 FR-005）。仅 mode == Grid 时读取；Row/Column 模式由自身语义决定。
    GridDirection gridDirection = GridDirection::Row;
    // 最大行/列数：0 = 未设置（自动）。
    //   约束优先：设置了任一约束即按约束排列——
    //     maxCols>0：列数 ≤ maxCols 固定，行数 = ceil(n/cols)（不丢窗口，FR-014）；
    //     maxRows>0：行数 ≤ maxRows 固定，列数 = ceil(n/rows)。
    //   无约束：保持比例的网格（非方阵）——
    //     Row/Grid（默认方向）：cols = round(√n)，rows = ceil(n/cols)
    //       （1→1×1, 2→2×1, 3→2×2, 5→3×2, 12→4×3, 13→4×4）；
    //     Column：转置（1→1×1, 2→1×2, 3→2×2, 5→2×3, 12→3×4, 13→4×4）。
    int        maxRows = 0;
    int        maxCols = 0;
    bool       sameSize = false;  // 保持相同宽高（FR-009）
    int        spacing = 6;       // 相邻子窗口间隙（px，FR-015 / SC-009；默认 6
                                  //   让 viewBg 容器中形成 6px 暗沟，配合 1px @border@ 子窗口边框
                                  //   在深色主题下可清晰分隔多个子窗口）
};

// 计算得到的网格结构
struct GridLayout {
    int rows = 0;
    int cols = 0;
};

class LayoutManager {
public:
    // FR-004~008：返回满足约束的行列数。
    //   约束优先：maxCols>0 -> 列数受限、行自适应；maxRows>0 -> 行数受限、列自适应
    //     （两者都设时列数固定、行数继续增长，FR-014 不丢窗口）。
    //   无约束 -> 保持比例的网格：
    //     Row/Grid：cols = round(√n)、rows = ceil(n/cols)（优先行）；
    //     Column：转置（优先列）。
    // windowCount <= 0 返回空网格。
    GridLayout computeGrid(int windowCount, const LayoutConfig& cfg) const;

    // 最后一个可见子窗口的跨度（QSize.width = 列跨度，.height = 行跨度）。
    // sameSize=true：恒为 1x1（各 cell 等大，末尾剩余 cell 留白）；
    // sameSize=false：最后一个子窗口从其位置延伸至网格右下角，占满剩余空间。
    QSize lastCellSpan(int windowCount, const GridLayout& grid, const LayoutConfig& cfg) const;

    // FR-009/FR-015：统一 cell 尺寸 = 可用尺寸扣除 (cols-1)*spacing / (rows-1)*spacing 后均分。
    QSize cellSize(const QSize& available, const GridLayout& grid, const LayoutConfig& cfg) const;

    // 第 index 个子窗口的矩形（含间隙偏移）；越界返回空矩形。
    QRect cellRect(const QSize& available, int index, const GridLayout& grid,
                   const LayoutConfig& cfg) const;
};

}  // namespace ui
}  // namespace perception

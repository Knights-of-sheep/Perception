#include "ui/subwindow/layout_preview_widget.h"

#include <QPainter>

#include "ui/theme/theme_manager.h"
#include "ui/theme/theme_types.h"

namespace perception {
namespace ui {

namespace {
// 预览内容区四边内边距（px）
constexpr int kPreviewMargin = 8;
}  // namespace

LayoutPreviewWidget::LayoutPreviewWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("layoutPreviewWidget"));
    setAttribute(Qt::WA_StyledBackground, true);  // 背景随 QSS（@viewBg@ 兜底）
    setMinimumSize(240, 76);
}

void LayoutPreviewWidget::setPreviewCount(int count) {
    if (previewCount_ == count) return;
    previewCount_ = count;
    update();
}

void LayoutPreviewWidget::setConfig(const LayoutConfig& cfg) {
    cfg_ = cfg;
    update();
}

QSize LayoutPreviewWidget::minimumSizeHint() const { return QSize(240, 76); }

void LayoutPreviewWidget::paintEvent(QPaintEvent*) {
    // 主题色与全局 QSS 同源（ThemeColors 单点定义，ui-guidelines §4.5）
    const auto* theme = ThemeManager::current();
    const QColor viewBg = theme->colors.viewBg;
    const QColor cell = theme->colors.panelBg;
    const QColor border = theme->colors.border;
    const QColor textWeak = theme->colors.textWeak;

    QPainter p(this);
    p.fillRect(rect(), viewBg);

    const int n = previewCount_;
    if (n <= 0) {
        // 空态提示（FR-013 Edge Case）
        p.setPen(textWeak);
        p.drawText(rect(), Qt::AlignCenter, tr("No subwindows"));
        return;
    }

    // 几何完全复用已测纯函数：预览与真实排布必然一致（契约 §6）
    const LayoutManager lm;
    const GridLayout grid = lm.computeGrid(n, cfg_);
    if (grid.rows <= 0 || grid.cols <= 0) return;

    const QSize avail(rect().width() - 2 * kPreviewMargin,
                      rect().height() - 2 * kPreviewMargin);
    p.setPen(border);
    const int cells = grid.rows * grid.cols;
    for (int i = 0; i < n && i < cells; ++i) {
        QRect r = lm.cellRect(avail, i, grid, cfg_);
        // 保持相同宽高未勾选：末尾子窗口延伸占满剩余空间（与容器 relayout 同语义，
        // 预览与真实排布一致；勾选时所有 cell 等大，两者预览不同）
        if (i == n - 1 && !cfg_.sameSize) {
            const QSize span = lm.lastCellSpan(n, grid, cfg_);
            const QSize cs = lm.cellSize(avail, grid, cfg_);
            r = QRect(r.x(), r.y(),
                      span.width() * cs.width() + (span.width() - 1) * cfg_.spacing,
                      span.height() * cs.height() + (span.height() - 1) * cfg_.spacing);
        }
        r.translate(kPreviewMargin, kPreviewMargin);
        p.fillRect(r, cell);
        p.drawRect(r.adjusted(0, 0, -1, -1));  // 1px 边框
    }
}

}  // namespace ui
}  // namespace perception

#include "ui/subwindow/subwindow_container.h"

#include <QGridLayout>
#include <QLabel>

#include "ui/subwindow/subwindow_view.h"

namespace perception {
namespace ui {

SubwindowContainer::SubwindowContainer(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("subwindowContainer"));
    setAttribute(Qt::WA_StyledBackground, true);  // QSS background-color: @viewBg@ 才能绘制
}

void SubwindowContainer::setEmptyHint(const QString& text) {
    if (!emptyHint_) {
        emptyHint_ = new QLabel(text, this);
        emptyHint_->setObjectName(QStringLiteral("centralPlaceholder"));  // 沿用空状态 QSS
        emptyHint_->setAlignment(Qt::AlignCenter);
    } else {
        emptyHint_->setText(text);
    }
    relayout();
}

void SubwindowContainer::addSubwindow(SubwindowView* view) {
    if (!view) return;
    if (!views_.contains(view)) {
        view->setParent(this);
        views_.append(view);
    }
    relayout();
    emit subwindowCountChanged(views_.size());
}

void SubwindowContainer::removeSubwindow(SubwindowView* view) {
    if (!view || !views_.contains(view)) return;
    if (maximized_ == view) exitMaximized();  // 先退出最大化，避免按钮状态残留
    views_.removeOne(view);
    hidden_.removeAll(view);
    view->hide();
    view->deleteLater();
    relayout();
    emit subwindowCountChanged(views_.size());
}

void SubwindowContainer::setLayoutConfig(const LayoutConfig& cfg) {
    const bool sameSizeChanged = (cfg.sameSize != cfg_.sameSize);
    cfg_ = cfg;
    if (sameSizeChanged) syncSameSize(cfg_.sameSize);
    relayout();
}

void SubwindowContainer::syncSameSize(bool enabled) {
    if (enabled) {
        // 开启：记录开启前独立尺寸（FR-010 恢复依据）
        for (auto* v : views_) {
            if (v->sizeBeforeSameSize().isNull()) v->setSizeBeforeSameSize(v->size());
        }
    } else {
        // 关闭：恢复独立尺寸
        for (auto* v : views_) {
            const QSize s = v->sizeBeforeSameSize();
            if (!s.isNull()) {
                v->setMinimumSize(0, 0);
                v->resize(s);
            }
        }
    }
}

void SubwindowContainer::setMaximized(SubwindowView* view) {
    if (!view || maximized_ == view) return;
    maximized_ = view;
    for (auto* v : views_) {
        v->setVisible(v == view);            // 其余隐藏，选中填满容器（FR-016）
        v->setMaximizedState(v == view);     // 标题栏切换为 还原/向前/向后
    }
    relayout();
}

void SubwindowContainer::exitMaximized() {
    if (!maximized_) return;
    maximized_ = nullptr;
    for (auto* v : views_) {
        v->setVisible(!hidden_.contains(v));  // 恢复排列，但保持用户隐藏的子窗口隐藏
        v->setMaximizedState(false);          // 标题栏切换为 最大化/隐藏
    }
    relayout();
}

void SubwindowContainer::hideSubwindow(SubwindowView* view) {
    if (!view || !views_.contains(view)) return;
    if (view == maximized_) return;  // 最大化对象不参与隐藏（标题栏无隐藏按钮）
    if (!hidden_.contains(view)) hidden_.append(view);
    view->setVisible(false);
    relayout();
}

void SubwindowContainer::showHiddenSubwindows() {
    if (hidden_.isEmpty()) return;
    for (auto* v : hidden_) v->setVisible(true);
    hidden_.clear();
    relayout();
}

void SubwindowContainer::cycleMaximized(int direction) {
    if (!maximized_) return;
    QVector<SubwindowView*> candidates;
    for (auto* v : views_) {
        if (!hidden_.contains(v)) candidates.append(v);
    }
    if (candidates.size() <= 1) return;
    const int idx = candidates.indexOf(maximized_);
    if (idx < 0) return;
    const int next = (idx + direction + candidates.size()) % candidates.size();
    setMaximized(candidates[next]);
}

void SubwindowContainer::relayout() {
    // 重建网格布局（先释放旧布局项，不销毁子窗口 widget）
    if (grid_) {
        while (QLayoutItem* item = grid_->takeAt(0)) delete item;
        delete grid_;
        grid_ = nullptr;
    }
    grid_ = new QGridLayout(this);
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setSpacing(cfg_.spacing);  // FR-015 / SC-009：间隙一致 ≥4px

    // 参与排列的子窗口：用户隐藏的不排；最大化时仅选中者（FR-016）
    QVector<SubwindowView*> visible;
    for (auto* v : views_) {
        if (hidden_.contains(v)) continue;
        if (!maximized_ || v == maximized_) visible.append(v);
    }

    if (visible.isEmpty()) {
        // 空状态提示（原中央占位语义）
        if (emptyHint_) {
            emptyHint_->setParent(this);
            grid_->addWidget(emptyHint_, 0, 0);
            emptyHint_->show();
        }
        return;
    }
    if (emptyHint_) emptyHint_->hide();

    const GridLayout grid = layoutManager_.computeGrid(visible.size(), cfg_);
    const QSize lastSpan = layoutManager_.lastCellSpan(visible.size(), grid, cfg_);
    const bool sameSize = cfg_.sameSize;
    for (int i = 0; i < visible.size(); ++i) {
        const int row = i / grid.cols;
        const int col = i % grid.cols;
        const bool isLast = (i == visible.size() - 1);
        if (isLast && !sameSize) {
            // 保持比例未勾选：最后一个子窗口占满剩余空间（末行/网格右下角）
            grid_->addWidget(visible[i], row, col, lastSpan.height(), lastSpan.width());
        } else {
            grid_->addWidget(visible[i], row, col);
        }
    }
    // 统一行/列 stretch：网格等分 → 相同宽高（FR-009 / SC-004，差异 <1px）
    for (int r = 0; r < grid.rows; ++r) grid_->setRowStretch(r, 1);
    for (int c = 0; c < grid.cols; ++c) grid_->setColumnStretch(c, 1);
    for (auto* v : visible) v->show();
}

}  // namespace ui
}  // namespace perception

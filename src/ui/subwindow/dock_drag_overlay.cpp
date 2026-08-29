#include "ui/subwindow/dock_drag_overlay.h"

#include "ui/theme/theme_catalog.h"  // ThemeDescriptor / ThemeColors（dockDropHighlight）
#include "ui/theme/theme_manager.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QPainter>

namespace perception {
namespace ui {

namespace {

// ---- Dock 拖拽放置高亮 widget（VSCode 风格分割线指示）----
// 高亮覆盖层：全窗口鼠标穿透，绘制"目标区域半透明填充 + 分割线 3px 实线"。
// 颜色来自主题 token dockDropHighlight（深色主题亮蓝系 / 浅色主题深蓝系 / 高对比青系，
// 参考 VSCode sash.activeBorder 与 editor.dropBackground）。
class DragOverlayWidget : public QWidget {
public:
    explicit DragOverlayWidget(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        hide();
    }
    void setHighlight(const QRect& fill, const QRect& line) {
        // 只重绘新旧高亮区域（含描边余量），避免每次 move 全窗重绘导致拖拽卡顿
        const QRect dirty =
            (fillRect_ | lineRect_ | fill | line).adjusted(-2, -2, 2, 2);
        fillRect_ = fill;
        lineRect_ = line;
        update(dirty);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        const QColor base = ThemeManager::current()->colors.dockDropHighlight;
        if (!fillRect_.isEmpty()) {
            QColor f = base;
            // 深色背景 alpha 46 视觉提升只有 +25~32，几乎看不见；
            // 提到 72（约 28%）后 fill 在深色面板上明显可见又不阻挡内容。
            f.setAlpha(72);
            p.fillRect(fillRect_, f);
        }
        if (!lineRect_.isEmpty()) {
            // 4px 实线 + 1px 反相描边：在深/浅背景下都保证线条清晰锐利
            // （仿 VSCode sash.activeBorder 在两种主题下的对比增强技巧）
            QColor edge = base.lightness() > 128 ? QColor(0, 0, 0, 160)
                                                 : QColor(255, 255, 255, 160);
            p.fillRect(lineRect_.adjusted(-1, 0, 1, 0), edge);  // 左右各扩 1px 描边
            p.fillRect(lineRect_, base);
        }
    }

private:
    QRect fillRect_;
    QRect lineRect_;
};

// ---- 分界线高亮细条（VSCode sash.activeBorder：用主题文字色，与背景永远对比清晰）----
class SashHighlightWidget : public QWidget {
public:
    explicit SashHighlightWidget(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        // 不透明 widget + paintEvent 画主题 WindowText 纯色：深色主题=白色亮条，
        // 亮色主题=深色暗条，永远与背景形成强对比；不依赖 WA_TranslucentBackground 的合成。
        setAutoFillBackground(false);
        setPalette(parent ? parent->palette() : QPalette());
        hide();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), palette().color(QPalette::WindowText));
    }
};

}  // namespace

DockDragOverlay::DockDragOverlay(QMainWindow* window, QObject* parent)
    : QObject(parent), window_(window) {}

QDockWidget* DockDragOverlay::fileDock() const {
    return window_->findChild<QDockWidget*>(QStringLiteral("fileDock"));
}

QDockWidget* DockDragOverlay::propertyDock() const {
    return window_->findChild<QDockWidget*>(QStringLiteral("propertyDock"));
}

QDockWidget* DockDragOverlay::pythonDock() const {
    return window_->findChild<QDockWidget*>(QStringLiteral("pythonConsoleDock"));
}

// ---- Dock 拖拽高亮（VSCode 风格：拖拽面板时目标分割线高亮）----
// 由 DockTitleBar::eventFilter 驱动：press→Candidate→(超过阈值)→Dragging，
// beginDockDrag 显示覆盖层，updateDockDrag 按鼠标位置更新高亮目标，
// endDockDrag 执行放置（addDockWidget 移动 dock 到目标区域）。
void DockDragOverlay::beginDockDrag(QDockWidget* dock) {
    dragDock_ = dock;
    if (!dragOverlayWidget_) {
        dragOverlayWidget_ = new DragOverlayWidget(window_);
        dragOverlayWidget_->setGeometry(window_->rect());
    }
    dragOverlayWidget_->raise();
    dragOverlayWidget_->show();
}

void DockDragOverlay::updateDockDrag(const QPoint& globalPos) {
    if (!dragOverlayWidget_ || !dragDock_) return;
    const QPoint pos = window_->mapFromGlobal(globalPos);
    const QRect r = window_->rect();
    const int zoneW = qRound(r.width() * 0.22);   // 左右放置带宽度（VSCode 式分带）
    const int zoneH = qRound(r.height() * 0.22);  // 底部放置带高度

    // 鼠标在哪个放置带？VSCode 行为：dock 标题栏被拖动时，分割线立即亮。
    // 鼠标在 dock 内部（不在任何 zone）时，默认高亮 dock 当前所在侧的分割线——
    // 这样用户拖动 fileDock 标题栏时 fileDock 右缘分割线立刻有高亮，
    // 把鼠标移到右/底 zone 时高亮位置随之切换。
    enum class Zone { Left, Right, Bottom };
    auto pickZone = [&]() {
        if (pos.x() < zoneW) return Zone::Left;
        if (pos.x() >= r.width() - zoneW) return Zone::Right;
        if (pos.y() >= r.height() - zoneH) return Zone::Bottom;
        // 鼠标在 dock 内部：保持 dock 当前所在区域
        const auto a = window_->dockWidgetArea(dragDock_);
        if (a == Qt::RightDockWidgetArea) return Zone::Right;
        if (a == Qt::BottomDockWidgetArea) return Zone::Bottom;
        return Zone::Left;  // 默认/左
    };
    const Zone zone = pickZone();

    QRect fill;  // 目标区域半透明填充
    QRect line;  // 分割线高亮条（3px）
    if (zone == Zone::Left) {
        const int x = (fileDock() && fileDock()->isVisible())
                          ? fileDock()->geometry().right()
                          : qRound(r.width() * 0.22);
        fill = QRect(0, 0, x, r.height());
        line = QRect(x - 1, 0, 3, r.height());
    } else if (zone == Zone::Right) {
        const int x = (propertyDock() && propertyDock()->isVisible())
                          ? propertyDock()->geometry().left()
                          : qRound(r.width() * 0.78);
        fill = QRect(x, 0, r.width() - x, r.height());
        line = QRect(x - 1, 0, 3, r.height());
    } else {
        const int y = (pythonDock() && pythonDock()->isVisible())
                          ? pythonDock()->geometry().top()
                          : qRound(r.height() * 0.78);
        fill = QRect(0, y, r.width(), r.height() - y);
        line = QRect(0, y - 1, r.width(), 3);
    }
    static_cast<DragOverlayWidget*>(dragOverlayWidget_)->setHighlight(fill, line);
}

void DockDragOverlay::endDockDrag(const QPoint& globalPos) {
    dragOverlayWidget_->hide();
    if (!dragDock_) return;
    const QPoint pos = window_->mapFromGlobal(globalPos);
    const int zoneW = qRound(window_->width() * 0.22);
    const int zoneH = qRound(window_->height() * 0.22);

    Qt::DockWidgetArea area = Qt::NoDockWidgetArea;
    if (pos.x() < zoneW) {
        area = Qt::LeftDockWidgetArea;
    } else if (pos.x() >= window_->width() - zoneW) {
        area = Qt::RightDockWidgetArea;
    } else if (pos.y() >= window_->height() - zoneH) {
        area = Qt::BottomDockWidgetArea;
    }
    // 目标区域允许停靠且与当前位置不同才移动（allowedAreas 限制：如左/右面板不可到底部）
    if (area != Qt::NoDockWidgetArea
        && (dragDock_->allowedAreas() & area)
        && window_->dockWidgetArea(dragDock_) != area) {
        window_->addDockWidget(area, dragDock_);
    }
    dragDock_ = nullptr;
}

// ---- 分隔条（dock 分隔条）resize 拖拽高亮 ----
// 背景：QMainWindow 的分隔条不是独立 widget，而是 QMainWindowLayout 绘制在 QMainWindow
// 上的 1px 边界线（qmainwindowlayout_p.h 的 windowEvent / paintSeparators），分隔条上的
// 鼠标事件直接派发给 QMainWindow（该区域没有子 widget 覆盖）。
// 方案：MainWindow::event() 在把事件转发给 QMainWindowLayout（windowEvent）之前检测分隔条
// 命中——此时尚未拖拽、dock geometry 准确，天然区分左右/上下多条分隔条；高亮条 = 细条
// overlay（只覆盖分隔条缝隙，局部重绘不卡顿），位置在 QMainWindowLayout 处理完 move 后
// 同步 → 与真实分隔条完全重合。
int DockDragOverlay::hitTest(const QPoint& pos) const {
    const int zone = 6;
    // 垂直分隔条：左侧 fileDock 右缘
    if (fileDock() && fileDock()->isVisible() && !fileDock()->isFloating()) {
        const QRect g = fileDock()->geometry();
        const int x = g.right() + 1;
        if (qAbs(pos.x() - x) <= zone && pos.y() >= g.top() - zone &&
            pos.y() <= g.bottom() + zone)
            return SashFileRight;
    }
    // 垂直分隔条：右侧 propertyDock 左缘
    if (propertyDock() && propertyDock()->isVisible() && !propertyDock()->isFloating()) {
        const QRect g = propertyDock()->geometry();
        const int x = g.left() - 1;
        if (qAbs(pos.x() - x) <= zone && pos.y() >= g.top() - zone &&
            pos.y() <= g.bottom() + zone)
            return SashPropertyLeft;
    }
    // 水平分隔条：底部 pythonDock 上缘
    if (pythonDock() && pythonDock()->isVisible() && !pythonDock()->isFloating()) {
        const QRect g = pythonDock()->geometry();
        const int y = g.top() - 1;
        if (qAbs(pos.y() - y) <= zone && pos.x() >= g.left() - zone &&
            pos.x() <= g.right() + zone)
            return SashPythonTop;
    }
    return SashMiss;
}

void DockDragOverlay::beginSashDrag(int hit) {
    sashDragging_ = true;
    sashHit_ = hit;
    if (!sashHighlightWidget_) sashHighlightWidget_ = new SashHighlightWidget(window_);
    sashHighlightWidget_->raise();
    sashHighlightWidget_->show();
    updateSashDrag();  // 初始位置 = 当前分隔条（无拖拽，准确）
}

void DockDragOverlay::updateSashDrag() {
    if (!sashDragging_ || !sashHighlightWidget_) return;
    const QRect line = highlightRect();  // 最新分隔条 rect（布局已更新）
    if (line.isEmpty()) {
        sashHighlightWidget_->hide();
        return;
    }
    // overlay 精确覆盖分隔条缝隙（4px 粗）：仅一个 widget 的局部重绘，代价极小
    sashHighlightWidget_->setGeometry(line);
}

void DockDragOverlay::endSashDrag() {
    if (!sashDragging_) return;
    sashDragging_ = false;
    sashHit_ = SashMiss;
    if (sashHighlightWidget_) sashHighlightWidget_->hide();
}

// 分隔条矩形：由命中类型 + 最新 dock geometry 计算。长度 = 分隔条实际长度
//（水平分隔条 = pythonDock 宽度；垂直分隔条 = 对应 dock 高度），与真实分隔条 100% 吻合。
QRect DockDragOverlay::highlightRect() const {
    switch (sashHit_) {
    case SashFileRight:
        if (fileDock() && fileDock()->isVisible() && !fileDock()->isFloating()) {
            const QRect g = fileDock()->geometry();
            // 垂直分隔条在 fileDock 右缘 +1px 处；4px 条中心对齐
            return QRect(g.right() - 1, g.top(), 4, g.height());
        }
        break;
    case SashPropertyLeft:
        if (propertyDock() && propertyDock()->isVisible() && !propertyDock()->isFloating()) {
            const QRect g = propertyDock()->geometry();
            // 垂直分隔条在 propertyDock 左缘 -1px 处
            return QRect(g.left() - 3, g.top(), 4, g.height());
        }
        break;
    case SashPythonTop:
        if (pythonDock() && pythonDock()->isVisible() && !pythonDock()->isFloating()) {
            const QRect g = pythonDock()->geometry();
            // 水平分隔条在 pythonDock 上缘 -1px 处；4px 条覆盖其上
            return QRect(g.left(), g.top() - 2, g.width(), 4);
        }
        break;
    default:
        break;
    }
    return QRect();
}

void DockDragOverlay::syncToWindowSize() {
    if (dragOverlayWidget_) dragOverlayWidget_->setGeometry(window_->rect());
}

}  // namespace ui
}  // namespace perception

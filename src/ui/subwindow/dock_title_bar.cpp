// ===== DockTitleBar / Dock 辅助实现 =====
// 006-constitution-refactor：自 MainWindow.cpp 提取（Dock 装配职责下放）。
// 依赖：MainWindow（拖拽高亮回调）、win_btn_icon（标题栏按钮 16px 矢量图标）。
#include "ui/subwindow/dock_title_bar.h"

#include "ui/MainWindow.h"
#include "ui/win_btn_icon.h"

#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSizeGrip>
#include <QStyle>
#include <QToolButton>

namespace perception {
namespace ui {

DockTitleBar::DockTitleBar(QDockWidget* parent)
    : QWidget(parent), dock_(parent) {
    setObjectName(QStringLiteral("dockTitleBar"));
    // 浮动态动态属性：供 QSS 在浮动时取消 wrap container 内层边框
    parent->setProperty("dockFloating", parent->isFloating());

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 4, 0);
    layout->setSpacing(2);

    titleLabel_ = new QLabel(this);
    titleLabel_->setObjectName(QStringLiteral("dockTitleLabel"));
    titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    minBtn_ = makeDockBtn(QStringLiteral("dockMinBtn"));
    minBtn_->setToolTip(tr("Minimize"));
    maxBtn_ = makeDockBtn(QStringLiteral("dockMaxBtn"));
    floatBtn_ = makeDockBtn(QStringLiteral("dockFloatBtn"));
    closeBtn_ = makeDockBtn(QStringLiteral("dockCloseBtn"));
    // 图标统一用 makeWinBtnIcon（16px 矢量；文本字符字形/基线不一，观感不齐）

    layout->addWidget(titleLabel_);
    layout->addWidget(minBtn_);
    layout->addWidget(maxBtn_);
    layout->addWidget(floatBtn_);
    layout->addWidget(closeBtn_);

    titleLabel_->setText(dock_->windowTitle());
    refreshByState();

    connect(minBtn_, &QToolButton::clicked, this, [this] {
        if (dock_->isFloating()) dock_->showMinimized();
    });
    connect(maxBtn_, &QToolButton::clicked, this, [this] {
        if (dock_->isMaximized()) {
            dock_->showNormal();              // 还原（浮动正常大小）
        } else {
            if (!dock_->isFloating()) dock_->setFloating(true);  // 停靠 → 先浮动
            dock_->showMaximized();           // 再最大化
        }
    });
    connect(floatBtn_, &QToolButton::clicked, this, [this] {
        dock_->setFloating(!dock_->isFloating());
    });
    connect(closeBtn_, &QToolButton::clicked, dock_, &QDockWidget::close);
    connect(dock_, &QDockWidget::windowTitleChanged,
            titleLabel_, &QLabel::setText);
    connect(dock_, &QDockWidget::featuresChanged,
            this, [this](QDockWidget::DockWidgetFeatures) { refreshByState(); });
    connect(dock_, &QDockWidget::topLevelChanged,
            this, &DockTitleBar::onTopLevelChanged);
    // 先于 QMainWindowLayout 安装拦截器，替换默认拖拽预览为"分割线高亮"指示
    installEventFilter(this);
}

bool DockTitleBar::eventFilter(QObject* obj, QEvent* ev) {
    if (obj != this) return QWidget::eventFilter(obj, ev);
    switch (ev->type()) {
    case QEvent::MouseButtonPress: {
        auto* e = static_cast<QMouseEvent*>(ev);
        if (e->button() != Qt::LeftButton) break;
        if (dock_->isFloating()) {
            pressGlobal_ = e->globalPos();
            winPos_ = dock_->pos();
            dragState_ = DragState::FloatMove;
            grabMouse();  // 移出标题栏后仍持续收到 move，窗口移动不中断
            return true;
        }
        pressGlobal_ = e->globalPos();
        dragState_ = DragState::Candidate;
        return true;  // 吞掉按下，避免 QMainWindowLayout 启动默认拖拽
    }
    case QEvent::MouseMove: {
        if (dragState_ == DragState::None) break;
        auto* e = static_cast<QMouseEvent*>(ev);
        const QPoint g = e->globalPos();
        if (dragState_ == DragState::Candidate) {
            if ((g - pressGlobal_).manhattanLength() >= QApplication::startDragDistance()) {
                dragState_ = DragState::Dragging;
                if (auto* mw = mainWindow()) {
                    mw->beginDockDrag(dock_);
                    mw->updateDockDrag(g);   // 立即更新高亮，避免极快 release 时 overlay 为空
                    grabMouse(Qt::ClosedHandCursor);  // 拖出标题栏后仍收到鼠标事件
                }
            }
            return true;
        }
        if (dragState_ == DragState::Dragging) {
            if (auto* mw = mainWindow()) mw->updateDockDrag(g);
            return true;
        }
        if (dragState_ == DragState::FloatMove) {
            dock_->move(winPos_ + g - pressGlobal_);
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (dragState_ == DragState::None) break;
        auto* e = static_cast<QMouseEvent*>(ev);
        if (dragState_ == DragState::Dragging) {
            if (auto* mw = mainWindow()) mw->endDockDrag(e->globalPos());
        }
        if (mouseGrabber() == this) releaseMouse();
        dragState_ = DragState::None;
        unsetCursor();
        return true;
    }
    default:
        break;
    }
    return QWidget::eventFilter(obj, ev);
}

void DockTitleBar::mouseDoubleClickEvent(QMouseEvent* e) {
    // 停靠态双击 = 分离浮动；浮动态双击 = 最大化/还原（Windows 惯例）
    if (dragState_ == DragState::None) {
        if (dock_->isFloating()) {
            if (dock_->isMaximized()) dock_->showNormal();
            else dock_->showMaximized();
        } else if (dock_->features() & QDockWidget::DockWidgetFloatable) {
            dock_->setFloating(true);
        }
    }
    QWidget::mouseDoubleClickEvent(e);
}

void DockTitleBar::changeEvent(QEvent* e) {
    if (e->type() == QEvent::PaletteChange ||
        e->type() == QEvent::ApplicationPaletteChange) {
        refreshIcons();          // 主题切换 → 窗口按钮图标颜色随主题刷新
    } else if (e->type() == QEvent::WindowStateChange) {
        refreshMaxBtn();         // 浮动态最大化/还原切换 → 更新图标
    }
    QWidget::changeEvent(e);
}

QToolButton* DockTitleBar::makeDockBtn(const QString& objName) {
    auto* btn = new QToolButton(this);
    btn->setObjectName(objName);
    btn->setFixedSize(20, 20);
    btn->setIconSize(QSize(16, 16));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setAutoRaise(true);
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

void DockTitleBar::refreshByState() {
    const auto f = dock_->features();
    const bool floating = dock_->isFloating();
    const bool floatable = f & QDockWidget::DockWidgetFloatable;
    floatBtn_->setVisible(floatable);
    closeBtn_->setVisible(f & QDockWidget::DockWidgetClosable);
    minBtn_->setVisible(floating && floatable);  // 仅浮动态有"最小化"
    maxBtn_->setVisible(floatable);
    refreshIcons();
}

void DockTitleBar::updateFloatIcon(bool floating) {
    // 浮动时按钮图标=恢复嵌入（↩）；停靠时=分离（⇅）
    const QPalette pal = dock_->palette();
    floatBtn_->setIcon(makeWinBtnIcon(floating ? WinBtnKind::Undock
                                               : WinBtnKind::FloatDock, pal));
    floatBtn_->setToolTip(floating ? tr("Re-dock") : tr("Undock"));
}

void DockTitleBar::refreshIcons() {
    const QPalette pal = dock_->palette();
    minBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Minimize, pal));
    closeBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Close, pal));
    updateFloatIcon(dock_->isFloating());
    refreshMaxBtn();
}

void DockTitleBar::refreshMaxBtn() {
    const bool max = dock_->isFloating() && dock_->isMaximized();
    maxBtn_->setIcon(makeWinBtnIcon(max ? WinBtnKind::Restore : WinBtnKind::Maximize,
                                    dock_->palette()));
    maxBtn_->setToolTip(max ? tr("Restore") : tr("Maximize"));
}

void DockTitleBar::onTopLevelChanged(bool topLevel) {
    // dockFloating 动态属性：传播到 dock 子树全部 widget，并 polish 它们。
    // QSS 属性选择器只约束该 widget 自身属性，且 unpolish/polish 单 widget
    // 不会自动重评估后代样式缓存——必须显式逐个 polish 才能让
    // "QDockWidget[dockFloating=...] QTreeWidget" 这类子选择器真正生效。
    const auto descendants = dock_->findChildren<QWidget*>();
    for (QWidget* w : descendants) {
        w->setProperty("dockFloating", topLevel);
    }
    dock_->setProperty("dockFloating", topLevel);
    if (auto* s = dock_->style()) {
        s->unpolish(dock_);
        s->polish(dock_);
        for (QWidget* w : descendants) {
            s->unpolish(w);
            s->polish(w);
        }
    }
    if (topLevel) {
        // 去系统标题栏（Frameless）：无边框/系统按钮，由本标题栏接管
        // （拖动/双击最大化/按钮控制/右下角缩放），窗口保留任务栏项（Qt::Window）。
        if (dock_->windowFlags().testFlag(Qt::Tool)) {
            dock_->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            dock_->show();  // setWindowFlags 会隐藏窗口，需重新显示
        }
    } else {
        // 嵌入主窗口：复位窗口状态，避免残留最大化/最小化
        dock_->setWindowState(Qt::WindowNoState);
    }
    refreshByState();
}

MainWindow* DockTitleBar::mainWindow() const {
    return qobject_cast<MainWindow*>(dock_->window());
}

void NoFocusRectDockStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                                         QPainter* p, const QWidget* w) const {
    if (pe == PE_FrameFocusRect) return;  // 跳过 dock focus 矩形
    QProxyStyle::drawPrimitive(pe, opt, p, w);
}

void applyNoFocusRectStyle(QDockWidget* dock) {
    if (dock) dock->setStyle(new NoFocusRectDockStyle(dock->style()));
}

QWidget* wrapWithSizeGrip(QWidget* content, QDockWidget* dock) {
    auto* container = new QWidget(dock);
    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);
    // content 横跨全部行/列：不再被 grip 行/列挤开。原先 grip 占独立行列
    // （row1/col1=12px），浮动时 content 被挤开：右侧 12px 与下侧 12px 露出
    // wrap 背景，导致内容左缘盖掉 wrap 左边框（左侧线中间缺失）且
    // pyshell 输入框下侧与外框之间出现 12px 间隔。
    grid->addWidget(content, 0, 0, 2, 2);
    grid->setRowStretch(0, 1);
    grid->setColumnStretch(0, 1);
    auto* grip = new QSizeGrip(container);
    grip->setObjectName(QStringLiteral("dockSizeGrip"));
    grip->setFixedSize(12, 12);
    grid->addWidget(grip, 1, 1);  // 叠放右下角，不占内容空间
    grip->setVisible(dock->isFloating());
    QObject::connect(dock, &QDockWidget::topLevelChanged, grip,
                     [grip](bool top) { grip->setVisible(top); });
    // 浮动时内容整体内缩 1px：wrap container 的 1px border（QSS 绘制在
    // rect 内侧环）不被内容 widget 覆盖，保证左/右/下缘描边完整；
    // docked 时紧贴 0px，由内容 widget 自身 border 充当 dock 外缘（现状不变）。
    QObject::connect(dock, &QDockWidget::topLevelChanged, grid,
                     [grid](bool top) {
                         grid->setContentsMargins(top ? 1 : 0, top ? 1 : 0,
                                                  top ? 1 : 0, top ? 1 : 0);
                     });
    return container;
}

}  // namespace ui
}  // namespace perception

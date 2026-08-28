#include "ui/subwindow/subwindow_view.h"
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"
#include "ui/win_btn_icon.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QToolButton>
#include <QVBoxLayout>

namespace perception {
namespace ui {

namespace {
constexpr const char* kViewObjectName      = "subwindowView";      // QSS 定位
constexpr const char* kTitleBarObjectName  = "subwindowTitleBar";
constexpr const char* kContentObjectName   = "subwindowContent";
constexpr int kTitleBarHeight = 26;        // 标题栏高度（布局 setFixedHeight 与 paintEvent 自绘背景共用）
// 标题区按钮统一尺寸（等高等宽，用户需求）
constexpr int kTitleButtonW = 26;
constexpr int kTitleButtonH = 24;
}  // namespace

SubwindowView::SubwindowView(const QString& title, QWidget* parent)
    : QFrame(parent), title_(title) {
    setObjectName(QString::fromUtf8(kViewObjectName));
    // 边框/背景由 paintEvent 自绘接管（QPainter），不依赖 QStyleSheetStyle 对 QFrame
    // border 几何宽度的 lineWidth 钳制（QSS 中 4px 实测只画 1px；QSS 仅作语义参考）。
    setFrameShape(QFrame::StyledPanel);
    setAttribute(Qt::WA_StyledBackground, true);        // 兼容保留：子控件样式按规则渲染
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    createUi();
    setMaximizedState(false);  // 初始：显示 最大化/隐藏
}

void SubwindowView::setTitle(const QString& title) {
    title_ = title;
    if (titleLabel_) titleLabel_->setText(title_);
}

void SubwindowView::paintEvent(QPaintEvent* event) {
    // 自绘背景、标题栏背景与边框（不依赖 QStyleSheetStyle，规避其对 QFrame border
    // 的 lineWidth 钳制）。titleBar_/content_ 均为透明子控件，不会遮挡自绘内容：
    //   - 背景：@panelBg@（整个子窗口）
    //   - 标题栏背景：@dockTitleBg@（从 y=边框宽度 处起，高 kTitleBarHeight，位于边框内侧）
    //   - 边框：选中 2px / 未选中 1px，颜色均 @border@（最后绘制，确保完整可见，
    //     不被子控件覆盖；此前 titleBar_ 不透明背景会盖住选中顶边框第二行）
    Q_UNUSED(event);
    const auto* t = ThemeManager::current();
    const QColor bg = t->colors.panelBg;
    const QColor bc = t->colors.border;
    const QColor tb = t->colors.dockTitleBg;
    const QColor bw = t->colors.borderWeak;
    const bool selected = property("selected").toBool();
    const int w = selected ? 2 : 1;
    QPainter p(this);
    p.fillRect(rect(), bg);                                        // 背景
    p.fillRect(0, w, width(), kTitleBarHeight, tb);                // 标题栏背景
    p.fillRect(0, w + kTitleBarHeight, width(), 1, bw);            // 标题栏底部分隔线（原 QSS border-bottom）
    p.fillRect(0, 0, width(), w, bc);                              // 上
    p.fillRect(0, height() - w, width(), w, bc);                   // 下
    p.fillRect(0, 0, w, height(), bc);                             // 左
    p.fillRect(width() - w, 0, w, height(), bc);                   // 右
}

bool SubwindowView::event(QEvent* event) {
    // 主题热切换（ThemeManager 应用新 QSS + QPalette）后刷新自绘颜色
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        update();
    }
    return QFrame::event(event);
}

void SubwindowView::createUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 自定义标题区（无系统标题栏：子控件 QWidget 天然无窗口装饰）。
    // WA_TranslucentBackground：背景由父 SubwindowView::paintEvent 自绘 @dockTitleBg@，
    // 否则不透明标题栏背景会覆盖自绘的选中顶边框（QSS 该规则仅作语义参考）。
    titleBar_ = new QWidget(this);
    titleBar_->setObjectName(QString::fromUtf8(kTitleBarObjectName));
    titleBar_->setFixedHeight(kTitleBarHeight);
    titleBar_->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* barLayout = new QHBoxLayout(titleBar_);
    barLayout->setContentsMargins(8, 0, 4, 0);
    barLayout->setSpacing(4);

    titleLabel_ = new QLabel(title_, titleBar_);
    titleLabel_->setObjectName(QStringLiteral("subwindowTitleLabel"));
    titleLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    titleBar_->installEventFilter(this);  // 双击标题区 = 最大化

    // 按钮集：全部统一尺寸 + QPainter 矢量图标（视觉等高等宽，随主题配色）。
    // 非最大化/非隐藏：最大化、隐藏、关闭；最大化：还原、向前、向后、关闭。
    const QPalette btnPal = palette();
    const auto makeBtn = [this, &btnPal](WinBtnKind kind, const QString& objectName) {
        auto* btn = new QToolButton(titleBar_);
        btn->setIcon(makeWinBtnIcon(kind, btnPal));
        btn->setIconSize(QSize(16, 16));
        btn->setAutoRaise(true);
        btn->setFixedSize(kTitleButtonW, kTitleButtonH);
        btn->setObjectName(objectName);
        return btn;
    };

    maximizeBtn_ = makeBtn(WinBtnKind::Maximize, QStringLiteral("subwindowMaxBtn"));
    maximizeBtn_->setToolTip(tr("Maximize"));

    hideBtn_ = makeBtn(WinBtnKind::Minimize, QStringLiteral("subwindowHideBtn"));
    hideBtn_->setToolTip(tr("Hide"));

    restoreBtn_ = makeBtn(WinBtnKind::Restore, QStringLiteral("subwindowRestoreBtn"));
    restoreBtn_->setToolTip(tr("Restore"));

    prevBtn_ = makeBtn(WinBtnKind::Prev, QStringLiteral("subwindowPrevBtn"));
    prevBtn_->setToolTip(tr("Previous Subwindow"));

    nextBtn_ = makeBtn(WinBtnKind::Next, QStringLiteral("subwindowNextBtn"));
    nextBtn_->setToolTip(tr("Next Subwindow"));

    closeBtn_ = makeBtn(WinBtnKind::Close, QStringLiteral("subwindowCloseBtn"));
    closeBtn_->setToolTip(tr("Close Subwindow"));

    barLayout->addWidget(titleLabel_, 1);
    barLayout->addWidget(maximizeBtn_);
    barLayout->addWidget(hideBtn_);
    barLayout->addWidget(restoreBtn_);
    barLayout->addWidget(prevBtn_);
    barLayout->addWidget(nextBtn_);
    barLayout->addWidget(closeBtn_);  // 关闭按钮始终显示（最右）
    layout->addWidget(titleBar_);

    // 内容区：占位渲染视图（FR-003；render 层未实装）。
    // WA_TranslucentBackground：不填充自身背景，让父控件自绘的 @panelBg@ 透出
    //（QSS 的 #subwindowContent viewBg 规则已移除，见 theme_template.qss 注释）。
    content_ = new QWidget(this);
    content_->setObjectName(QString::fromUtf8(kContentObjectName));
    content_->setAttribute(Qt::WA_TranslucentBackground, true);
    auto* contentLayout = new QVBoxLayout(content_);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    auto* placeholder = new QLabel(tr("(render view placeholder)"), content_);
    placeholder->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(placeholder);
    layout->addWidget(content_, 1);

    // 标题区按钮动作
    connect(maximizeBtn_, &QToolButton::clicked, this, &SubwindowView::maximizeRequested);
    connect(hideBtn_, &QToolButton::clicked, this, &SubwindowView::hideRequested);
    connect(restoreBtn_, &QToolButton::clicked, this, &SubwindowView::restoreRequested);
    connect(prevBtn_, &QToolButton::clicked, this, &SubwindowView::prevRequested);
    connect(nextBtn_, &QToolButton::clicked, this, &SubwindowView::nextRequested);
    connect(closeBtn_, &QToolButton::clicked, this, &SubwindowView::closeRequested);
}

void SubwindowView::setMaximizedState(bool maximized) {
    maximized_ = maximized;
    maximizeBtn_->setVisible(!maximized);
    hideBtn_->setVisible(!maximized);
    restoreBtn_->setVisible(maximized);
    prevBtn_->setVisible(maximized);
    nextBtn_->setVisible(maximized);
}

bool SubwindowView::eventFilter(QObject* watched, QEvent* event) {
    if (watched == titleBar_) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            emit maximizeRequested();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            emit selected();  // 点击标题栏任意区域也选中该子窗口（整窗可选中）
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SubwindowView::mousePressEvent(QMouseEvent* event) {
    emit selected();  // 单击任意区域 = 选中（spec Assumptions）
    QWidget::mousePressEvent(event);
}

void SubwindowView::contextMenuEvent(QContextMenuEvent* event) {
    emit selected();
    QMenu menu(this);
    QAction* chosen = nullptr;
    if (maximized_) {
        QAction* restoreAction = menu.addAction(tr("Restore"));
        QAction* prevAction = menu.addAction(tr("Previous Subwindow"));
        QAction* nextAction = menu.addAction(tr("Next Subwindow"));
        menu.addSeparator();
        QAction* closeAction = menu.addAction(tr("Close Subwindow"));
        chosen = menu.exec(event->globalPos());
        if (chosen == restoreAction) {
            emit restoreRequested();
        } else if (chosen == prevAction) {
            emit prevRequested();
        } else if (chosen == nextAction) {
            emit nextRequested();
        } else if (chosen == closeAction) {
            emit closeRequested();
        }
    } else {
        QAction* maximizeAction = menu.addAction(tr("Maximize"));
        QAction* hideAction = menu.addAction(tr("Hide"));
        menu.addSeparator();
        QAction* closeAction = menu.addAction(tr("Close Subwindow"));
        chosen = menu.exec(event->globalPos());
        if (chosen == maximizeAction) {
            emit maximizeRequested();
        } else if (chosen == hideAction) {
            emit hideRequested();
        } else if (chosen == closeAction) {
            emit closeRequested();
        }
    }
}

}  // namespace ui
}  // namespace perception

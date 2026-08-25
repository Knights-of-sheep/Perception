// ===== Perception 主窗口实现（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// 规范：docs/design/ui-guidelines.md §5 布局范式 / §6 交互规范。
#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QProxyStyle>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QScreen>
#include <QSizeGrip>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX  // 避免与 Qt 的 qMin/qMax 及标准库 min/max 宏冲突
#endif
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM / GET_Y_LPARAM
#endif



#include "core/log/logger.h"

#include "ui/action_icon_map.h"
#include "ui/console/PythonConsole.h"
#include "ui/theme/icon_factory.h"
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"

namespace perception {
namespace ui {

namespace {
constexpr const char* kFileDockObjectName     = "fileDock";        // QSS 定位（ui-guidelines §4.1）
constexpr const char* kPropertyDockObjectName = "propertyDock";
constexpr const char* kPythonDockObjectName   = "pythonConsoleDock";  // 布局记忆用
constexpr const char* kSettingsLayoutKey      = "mainWindow/layout";
constexpr const char* kSettingsGeometryKey    = "mainWindow/geometry";
constexpr const char* kEmptyTreeText          = "(尚未加载文件)";
constexpr const char* kEmptyPropertyText      = "(选择对象查看属性)";

// 日志级别持久化 key（FR-013）：全局单一矩阵（FR-012 修订，不再区分控制台/文件）。
// 兼容：旧版本曾分别持久化 log/console/<LEVEL> 与 log/file/<LEVEL>；升级后优先读新
// 全局 key，未设置时回退到旧 log/console/<LEVEL>，保证既有用户设置不丢。
constexpr const char* kLogLevelPrefix         = "log/level/";
constexpr const char* kLogLegacyConsolePrefix = "log/console/";
constexpr const char* kLogVtkEnabled          = "log/vtkEnabled";
constexpr const char* kLogPathKey       = "log/path";  // 用户配置的日志文件路径（FR-016）

// 003-install-icon-bars：动作图标统一构造（IconFactory 五态派生，T-04/T-06）
QIcon makeActionIcon(const QString& iconId) {
    const auto* t = ThemeManager::current();
    return theme::IconFactory::actionIcon(iconId, t->colors.textWeak, t->colors.textOnSelection,
                                          t->colors.textDisabled, t->colors.accent);
}

// ---- 窗口控制按钮图标（统一 16px 矢量绘制；文本字符字形/基线不一，观感不齐）----
enum class WinBtnKind { Minimize, Maximize, Restore, Close, FloatDock, Undock };

void drawWinBtnIcon(QPainter& p, WinBtnKind kind, const QColor& color) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    switch (kind) {
    case WinBtnKind::Minimize:
        // 横线垂直居中（与其他图标 centerY 对齐，避免按钮排布不齐平）
        p.drawLine(QPointF(3, 8.5), QPointF(13, 8.5));
        break;
    case WinBtnKind::Maximize:
        p.drawRect(QRectF(3.2, 3.2, 9.6, 9.6));  // 方框
        break;
    case WinBtnKind::Restore:
        // 两个重叠小方框（Windows 还原惯例：后框 + 前框）
        p.drawRect(QRectF(2.4, 5.4, 8.2, 8.2));
        p.drawRect(QRectF(5.4, 2.4, 8.2, 8.2));
        break;
    case WinBtnKind::Close:
        // 对角交叉
        p.drawLine(QPointF(3.6, 3.6), QPointF(12.4, 12.4));
        p.drawLine(QPointF(12.4, 3.6), QPointF(3.6, 12.4));
        break;
    case WinBtnKind::FloatDock:
        // 分离：上下对三角（⇅ 语义，停靠面板分离为浮动窗口）
        p.drawPolyline(QPolygonF() << QPointF(8, 3.2) << QPointF(4.6, 7.4)
                                   << QPointF(11.4, 7.4) << QPointF(8, 3.2));
        p.drawPolyline(QPolygonF() << QPointF(8, 12.8) << QPointF(4.6, 8.6)
                                   << QPointF(11.4, 8.6) << QPointF(8, 12.8));
        break;
    case WinBtnKind::Undock:
        // 恢复嵌入：L 形返回箭头（↩ 语义，浮动窗口回到主窗口停靠）
        p.drawLine(QPointF(12.5, 4.2), QPointF(12.5, 9.2));
        p.drawLine(QPointF(12.5, 9.2), QPointF(4.8, 9.2));
        p.drawLine(QPointF(4.8, 9.2), QPointF(7.6, 6.8));
        p.drawLine(QPointF(4.8, 9.2), QPointF(7.6, 11.6));
        break;
    }
    p.restore();
}

QIcon makeWinBtnIcon(WinBtnKind kind, const QPalette& pal) {
    const QColor normal = pal.color(QPalette::WindowText);
    // 关闭按钮 hover 背景为红色（QSS @dangerHoverBg@），图标取白色保证对比；
    // 其余按钮 hover 沿用主文字色。
    const QColor active = (kind == WinBtnKind::Close)
                              ? pal.color(QPalette::BrightText)
                              : pal.color(QPalette::WindowText);
    QIcon icon;
    const int sizes[] = {16, 32};  // 兼容高 DPI 缩放
    for (int s : sizes) {
        QPixmap base(s, s);
        base.fill(Qt::transparent);
        { QPainter p(&base); drawWinBtnIcon(p, kind, normal); }
        QPixmap act(s, s);
        act.fill(Qt::transparent);
        { QPainter p(&act); drawWinBtnIcon(p, kind, active); }
        icon.addPixmap(base);                             // Normal / Off
        icon.addPixmap(act, QIcon::Active, QIcon::Off);   // hover / 按下
    }
    return icon;
}

// ---- 自定义 Dock 标题栏 ----
// 背景：Qt5 + Fusion 风格下，浮动的 QDockWidget 不绘制 normal-button 子控件，
//       导致"恢复嵌入"按钮缺失，用户分离后找不到还原入口。
// 方案：setTitleBarWidget(DockTitleBar) 接管标题栏——浮动时彻底无系统标题栏
//       （Qt::FramelessWindowHint），标题栏自身承担拖动移动 / 双击最大化/还原；
//       按钮集：停靠态 = 最大化 + 分离 + 关闭；浮动态 = 最小化 + 最大化/还原 +
//       恢复嵌入 + 关闭，全用显式 QToolButton，停靠/浮动均稳定可见。
class DockTitleBar : public QWidget {
public:
    explicit DockTitleBar(QDockWidget* parent)
        : QWidget(parent), dock_(parent) {
        setObjectName(QStringLiteral("dockTitleBar"));

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 0, 4, 0);
        layout->setSpacing(2);

        titleLabel_ = new QLabel(this);
        titleLabel_->setObjectName(QStringLiteral("dockTitleLabel"));
        titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        minBtn_ = makeDockBtn(QStringLiteral("dockMinBtn"));
        minBtn_->setToolTip(tr("最小化"));
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

protected:
    // 停靠态：press→超过阈值→Dragging，通知主窗口显示放置高亮；release 执行放置。
    // 浮动态：press/move 直接移动浮动窗口。
    bool eventFilter(QObject* obj, QEvent* ev) override {
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

    void mouseDoubleClickEvent(QMouseEvent* e) override {
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

    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::PaletteChange ||
            e->type() == QEvent::ApplicationPaletteChange) {
            refreshIcons();          // 主题切换 → 窗口按钮图标颜色随主题刷新
        } else if (e->type() == QEvent::WindowStateChange) {
            refreshMaxBtn();         // 浮动态最大化/还原切换 → 更新图标
        }
        QWidget::changeEvent(e);
    }

private:
    QToolButton* makeDockBtn(const QString& objName) {
        auto* btn = new QToolButton(this);
        btn->setObjectName(objName);
        btn->setFixedSize(20, 20);
        btn->setIconSize(QSize(16, 16));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        return btn;
    }

    void refreshByState() {
        const auto f = dock_->features();
        const bool floating = dock_->isFloating();
        const bool floatable = f & QDockWidget::DockWidgetFloatable;
        floatBtn_->setVisible(floatable);
        closeBtn_->setVisible(f & QDockWidget::DockWidgetClosable);
        minBtn_->setVisible(floating && floatable);  // 仅浮动态有"最小化"
        maxBtn_->setVisible(floatable);
        refreshIcons();
    }
    void updateFloatIcon(bool floating) {
        // 浮动时按钮图标=恢复嵌入（↩）；停靠时=分离（⇅）
        const QPalette pal = dock_->palette();
        floatBtn_->setIcon(makeWinBtnIcon(floating ? WinBtnKind::Undock
                                                   : WinBtnKind::FloatDock, pal));
        floatBtn_->setToolTip(floating ? tr("恢复嵌入") : tr("分离为浮动窗口"));
    }
    void refreshIcons() {
        const QPalette pal = dock_->palette();
        minBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Minimize, pal));
        closeBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Close, pal));
        updateFloatIcon(dock_->isFloating());
        refreshMaxBtn();
    }
    void refreshMaxBtn() {
        const bool max = dock_->isFloating() && dock_->isMaximized();
        maxBtn_->setIcon(makeWinBtnIcon(max ? WinBtnKind::Restore : WinBtnKind::Maximize,
                                        dock_->palette()));
        maxBtn_->setToolTip(max ? tr("还原") : tr("最大化"));
    }
    void onTopLevelChanged(bool topLevel) {
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

    QDockWidget*  dock_;
    QLabel*       titleLabel_;
    QToolButton*  minBtn_;
    QToolButton*  maxBtn_;
    QToolButton*  floatBtn_;
    QToolButton*  closeBtn_;

    // 自定义拖拽状态（停靠=分割线高亮；浮动=窗口移动）
    enum class DragState { None, Candidate, Dragging, FloatMove };
    DragState dragState_ = DragState::None;
    QPoint    pressGlobal_;
    QPoint    winPos_;  // 浮动态拖动前的窗口位置

    MainWindow* mainWindow() const {
        return qobject_cast<MainWindow*>(dock_->window());
    }
};

// ---- Dock focus rect 抑制（避免点击 dock 内容时 dock 出现蓝色 focus 边框）----
// Qt Fusion 风格在 QDockWidget 获得键盘焦点时自动绘制 1-2px focus rect（QStyle::PE_FrameFocusRect），
// 包裹整个 dock 外缘——视觉上像"dock 被高亮选中"，与设计语言不符。
// 用代理样式仅对 QDockWidget 抑制该元素，不影响其他 widget 的 focus 反馈。
class NoFocusRectDockStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                       QPainter* p, const QWidget* w) const override {
        if (pe == PE_FrameFocusRect) return;  // 跳过 dock focus 矩形
        QProxyStyle::drawPrimitive(pe, opt, p, w);
    }
};
static void applyNoFocusRectStyle(QDockWidget* dock) {
    if (dock) dock->setStyle(new NoFocusRectDockStyle(dock->style()));
}

// ---- Dock 拖拽放置高亮（VSCode 风格分割线指示）----
// 高亮覆盖层：全窗口鼠标穿透，绘制"目标区域半透明填充 + 分割线 3px 实线"。
// 颜色来自主题 token dockDropHighlight（深色主题亮蓝系 / 浅色主题深蓝系 / 高对比青系，
// 参考 VSCode sash.activeBorder 与 editor.dropBackground）。
class DockDragOverlay : public QWidget {
public:
    explicit DockDragOverlay(QWidget* parent) : QWidget(parent) {
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

// ---- 浮动窗口右下角调整大小手柄（去系统标题栏后失去边缘 resize，QSizeGrip 补位）----
static QWidget* wrapWithSizeGrip(QWidget* content, QDockWidget* dock) {
    auto* container = new QWidget(dock);
    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);
    grid->addWidget(content, 0, 0);
    auto* grip = new QSizeGrip(container);
    grip->setObjectName(QStringLiteral("dockSizeGrip"));
    grid->addWidget(grip, 1, 1);  // 贴右下角
    grid->setRowStretch(0, 1);
    grid->setColumnStretch(0, 1);
    grip->setVisible(dock->isFloating());
    QObject::connect(dock, &QDockWidget::topLevelChanged, grip,
                     [grip](bool top) { grip->setVisible(top); });
    return container;
}

// ---- 分界线高亮细条（VSCode sash.activeBorder：用主题文字色，与背景永远对比清晰）----
class SashHighlight : public QWidget {
public:
    explicit SashHighlight(QWidget* parent) : QWidget(parent) {
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Perception"));
    // 无边框：舍弃系统标题栏，标题/窗口按钮与菜单栏同一行（createTitleBar 组装）
    // 最大化时通过 WM_GETMINMAXINFO 限制到工作区，避免覆盖任务栏
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1360, 860);

    createActions();
    createMenus();
    createTitleBar();  // 需在 createMenus 之后（组装 menuBar()）
    createToolbars();
    createDocks();
    createCentralArea();
    createStatusBar();

    // Dock 布局记忆（ui-guidelines §5.1：下次打开还原布局）
    QSettings settings;
    restoreGeometry(settings.value(kSettingsGeometryKey).toByteArray());
    restoreState(settings.value(kSettingsLayoutKey).toByteArray());

    setAcceptDrops(true);  // 拖放打开（ui-guidelines §6）
    updateEmptyHints();

}

// ---- 分界线（dock 边缘分隔条）resize 拖拽高亮 ----
// 背景：QMainWindow 的分隔条不是独立 widget，而是 QMainWindowLayout 绘制在 QMainWindow
// 上的 1px 边界线（qmainwindowlayout_p.h 的 windowEvent / paintSeparators），分隔条上的
// 鼠标事件直接派发给 QMainWindow（该区域没有子 widget 覆盖）。
// 方案：MainWindow::event() 在把事件转发给 QMainWindowLayout（windowEvent）之前检测分隔条
// 命中——此时尚未拖拽、dock geometry 准确，天然区分左右/上下多条分隔条（不存在旧方案
// "拖右亮左"的歧义）；高亮条 = 细条 overlay（只覆盖分隔条缝隙，局部重绘不卡顿），位置在
// QMainWindowLayout 处理完 move 后同步 → 与真实分隔条完全重合。
int MainWindow::sashHitTest(const QPoint& pos) const {
    const int zone = 6;
    // 垂直分隔条：左侧 fileDock 右缘
    if (fileDock_ && fileDock_->isVisible() && !fileDock_->isFloating()) {
        const QRect g = fileDock_->geometry();
        const int x = g.right() + 1;
        if (qAbs(pos.x() - x) <= zone && pos.y() >= g.top() - zone &&
            pos.y() <= g.bottom() + zone)
            return SashFileRight;
    }
    // 垂直分隔条：右侧 propertyDock 左缘
    if (propertyDock_ && propertyDock_->isVisible() && !propertyDock_->isFloating()) {
        const QRect g = propertyDock_->geometry();
        const int x = g.left() - 1;
        if (qAbs(pos.x() - x) <= zone && pos.y() >= g.top() - zone &&
            pos.y() <= g.bottom() + zone)
            return SashPropertyLeft;
    }
    // 水平分隔条：底部 pythonDock 上缘
    if (pythonDock_ && pythonDock_->isVisible() && !pythonDock_->isFloating()) {
        const QRect g = pythonDock_->geometry();
        const int y = g.top() - 1;
        if (qAbs(pos.y() - y) <= zone && pos.x() >= g.left() - zone &&
            pos.x() <= g.right() + zone)
            return SashPythonTop;
    }
    return SashMiss;
}
void MainWindow::createActions() {
    openAction_ = new QAction(tr("打开(&O)..."), this);
    openAction_->setShortcut(QKeySequence::Open);  // Ctrl+O
    openAction_->setStatusTip(tr("打开 .plt / .csv 数据文件"));
    setActionIcon(openAction_, "file-open");  // 003：契约图标（下同）
    connect(openAction_, &QAction::triggered, this, &MainWindow::openFile);

    exportAction_ = new QAction(tr("导出命令脚本(&E)..."), this);
    exportAction_->setShortcut(QKeySequence::SaveAs);  // Ctrl+Shift+S
    exportAction_->setEnabled(false);  // 契约 §1：禁用态保持（导出命令脚本未实现）
    setActionIcon(exportAction_, "file-export-data");
    connect(exportAction_, &QAction::triggered, this, [this] {
        statusBar()->showMessage(tr("导出命令脚本将在后续版本提供"), 3000);
    });

    // 导出主界面图片（与"打开"同级，文件菜单；M3a 截图调试验证用）
    exportImageAction_ = new QAction(tr("导出主界面图片(&I)..."), this);
    exportImageAction_->setShortcut(QKeySequence("Ctrl+I"));
    exportImageAction_->setStatusTip(tr("将当前主窗口（含菜单/Dock/状态栏）保存为 PNG 图片"));
    setActionIcon(exportImageAction_, "file-export-screenshot");
    connect(exportImageAction_, &QAction::triggered, this, &MainWindow::exportMainWindowImage);

    exitAction_ = new QAction(tr("退出(&X)"), this);
    exitAction_->setShortcut(QKeySequence::Quit);
    setActionIcon(exitAction_, "file-close");
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    toggleFileDockAction_ = new QAction(tr("数据集面板"), this);
    toggleFileDockAction_->setCheckable(true);
    toggleFileDockAction_->setShortcut(QKeySequence("Ctrl+1"));
    setActionIcon(toggleFileDockAction_, "view-panel-data");

    togglePropertyDockAction_ = new QAction(tr("属性面板"), this);
    togglePropertyDockAction_->setCheckable(true);
    togglePropertyDockAction_->setShortcut(QKeySequence("Ctrl+2"));
    setActionIcon(togglePropertyDockAction_, "view-panel-property");

    togglePythonConsoleAction_ = new QAction(tr("Python 控制台"), this);
    togglePythonConsoleAction_->setCheckable(true);
    togglePythonConsoleAction_->setShortcut(QKeySequence("Ctrl+`"));
    setActionIcon(togglePythonConsoleAction_, "view-panel-console");

    resetLayoutAction_ = new QAction(tr("重置布局"), this);
    resetLayoutAction_->setShortcut(QKeySequence("Ctrl+Shift+L"));
    resetLayoutAction_->setStatusTip(tr("恢复左侧数据集、右侧属性的默认布局"));
    setActionIcon(resetLayoutAction_, "view-reset-camera");

    // 003：未实现功能占位动作（FR-011：禁用态 + tooltip 明确提示，不连接功能槽）
    undoAction_ = new QAction(tr("撤销(&U)"), this);
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);
    undoAction_->setStatusTip(tr("撤销上一步操作（功能即将推出）"));
    setActionIcon(undoAction_, "edit-undo");

    redoAction_ = new QAction(tr("重做(&R)"), this);
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false);
    redoAction_->setStatusTip(tr("重做被撤销的操作（功能即将推出）"));
    setActionIcon(redoAction_, "edit-redo");

    loadScriptAction_ = new QAction(tr("加载脚本(&L)..."), this);
    loadScriptAction_->setEnabled(false);
    loadScriptAction_->setStatusTip(tr("加载 Python 脚本（功能即将推出）"));
    setActionIcon(loadScriptAction_, "file-load-script");

    recordScreenAction_ = new QAction(tr("主界面视频录制(&V)"), this);
    recordScreenAction_->setEnabled(false);
    recordScreenAction_->setStatusTip(tr("录制主界面为视频（功能即将推出）"));
    setActionIcon(recordScreenAction_, "file-record-screen");

    refreshAction_ = new QAction(tr("刷新(&F)"), this);
    refreshAction_->setEnabled(false);
    refreshAction_->setStatusTip(tr("刷新当前视图（功能即将推出）"));
    setActionIcon(refreshAction_, "view-refresh");

    helpAction_ = new QAction(tr("帮助(&H)"), this);
    helpAction_->setStatusTip(tr("查看帮助文档"));
    setActionIcon(helpAction_, "tools-help");
    connect(helpAction_, &QAction::triggered, this, [this] {
        QMessageBox::information(this, tr("帮助"), tr("帮助文档将在后续版本提供"));
    });

    aboutAction_ = new QAction(tr("关于(&A)..."), this);
    setActionIcon(aboutAction_, "tools-about");
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::about);
}

// ---- 菜单栏 ----
void MainWindow::createMenus() {
    // 禁用 Windows 原生菜单栏：否则菜单栏由系统绘制（浅色、不走 QSS），
    // 与深色主题割裂，窗口顶部会呈现"两层"的观感。
    menuBar()->setNativeMenuBar(false);

    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(openAction_);
    fileMenu->addAction(exportAction_);  // 导出命令脚本（禁用态保持，契约 §1）
    fileMenu->addAction(exportImageAction_);  // 导出主界面图片（与"打开"同级，调试快照）
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction_);

    // 编辑（003：撤销/重做占位，FR-011 禁用态）
    QMenu* editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    editMenu->addAction(undoAction_);
    editMenu->addAction(redoAction_);

    QMenu* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    viewMenu->addAction(toggleFileDockAction_);
    viewMenu->addAction(togglePropertyDockAction_);
    viewMenu->addAction(togglePythonConsoleAction_);
    viewMenu->addSeparator();
    viewMenu->addAction(resetLayoutAction_);

    // 主题（25 套，按 family 分组；勾选当前项，点击即热切换）
    QMenu* themeMenu = menuBar()->addMenu(tr("主题(&T)"));
    themeGroup_ = new QActionGroup(this);
    themeGroup_->setExclusive(true);
    QString currentFamily;
    const auto themes = ThemeManager::themes();
    for (int i = 0; i < ThemeManager::themeCount(); ++i) {
        const auto* t = themes + i;
        const QString family = QString::fromUtf8(t->family);
        if (i > 0 && family != currentFamily) {
            themeMenu->addSeparator();  // 深色 / 浅色 / 高对比 分组
        }
        currentFamily = family;

        QAction* act = themeMenu->addAction(QString::fromUtf8(t->name));
        act->setCheckable(true);
        act->setData(QString::fromLatin1(t->id));
        act->setStatusTip(tr("切换主题：%1（%2）")
                              .arg(QString::fromUtf8(t->name), family));
        themeGroup_->addAction(act);
        themeActions_.append(act);
        // triggered(bool) 与槽 applyTheme(QString) 参数不匹配，用 lambda 转发
        connect(act, &QAction::triggered, this, [this, act] {
            applyTheme(act->data().toString());
        });
    }
    // 勾选当前主题
    const QString curId = ThemeManager::currentThemeId();
    for (QAction* act : themeActions_) {
        if (act->data().toString() == curId) { act->setChecked(true); break; }
    }

    // ---- 设置 → 日志级别（FR-002/012/013 修订）----
    // 全局单一矩阵：同一组级别同时作用于 终端(控制台) / 日志面板 / 文件 全部 sink，
    // 不再区分"控制台/文件"（用户反馈：级别设置应全局一致，FR-012 修订）。
    QMenu* settingsMenu = menuBar()->addMenu(tr("设置(&S)"));
    QMenu* logLevelMenu = settingsMenu->addMenu(tr("日志级别(&L)"));
    logLevelMenu->setToolTipsVisible(true);

    const char* const kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    // 批量开关：解决"单级别勾选状态不一致/用户找不到入口"的痛点。
    // 置于级别列表顶部，与具体级别分隔，语义直观。
    logLevelMenu->addSeparator();
    allLevelsAction_ = logLevelMenu->addAction(tr("全部启用"));
    noneLevelsAction_ = logLevelMenu->addAction(tr("全部禁用"));
    connect(allLevelsAction_, &QAction::triggered, this, [this] { onAllLevels(true); });
    connect(noneLevelsAction_, &QAction::triggered, this, [this] { onAllLevels(false); });

    for (int i = 0; i < 5; ++i) {
        QAction* c = logLevelMenu->addAction(QString::fromLatin1(kLevelNames[i]));
        c->setCheckable(true);
        c->setChecked(true);  // 默认矩阵：DEBUG 关、其余开
        if (i == 0) c->setChecked(false);
        c->setData(i);  // LogLevel 索引
        connect(c, &QAction::toggled, this, &MainWindow::onLevelToggled);
        levelActions_.append(c);
    }

    // VTK 日志拦截开关（FR-011；VTK 未引入，仅配置项）
    vtkLogAction_ = settingsMenu->addAction(tr("VTK 日志拦截(&V)"));
    vtkLogAction_->setCheckable(true);
    vtkLogAction_->setChecked(true);  // 默认开启
    setActionIcon(vtkLogAction_, "tools-settings");  // 003：契约图标
    connect(vtkLogAction_, &QAction::toggled, this, [](bool checked) {
        QSettings settings;
        settings.setValue(QLatin1String(kLogVtkEnabled), checked);
    });

    // ---- 日志路径可达性（FR-014）：只读路径 + 一键打开日志目录 ----
    // 日志写入 %APPDATA%\Perception\logs（隐藏目录），用户无从查找，
    // 故在设置菜单直接展示完整路径并给出"打开日志目录"直达入口。
    settingsMenu->addSeparator();
    logPathAction_ = settingsMenu->addAction(tr("日志文件：未配置"));
    logPathAction_->setEnabled(false);  // 只读展示（可选中复制），路径由 main.cpp 注入后更新
    openLogDirAction_ = settingsMenu->addAction(tr("打开日志目录(&O)"));
    openLogDirAction_->setEnabled(false);  // 路径注入前不可用
    setActionIcon(openLogDirAction_, "tools-settings");  // 003：契约图标
    connect(openLogDirAction_, &QAction::triggered, this, &MainWindow::openLogDir);

    // 日志路径可配置（FR-016）：选择目录后迁移旧日志并持久化
    setLogPathAction_ = settingsMenu->addAction(tr("设置日志路径...(&P)"));
    setLogPathAction_->setEnabled(false);  // 路径注入前不可用
    setActionIcon(setLogPathAction_, "tools-settings");  // 003：契约图标
    connect(setLogPathAction_, &QAction::triggered, this, &MainWindow::setLogPath);
    // 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档
    clearLogAction_ = settingsMenu->addAction(tr("清除历史日志(&C)"));
    clearLogAction_->setEnabled(false);
    setActionIcon(clearLogAction_, "edit-delete-selection");  // 003：契约图标
    connect(clearLogAction_, &QAction::triggered, this, &MainWindow::clearLogHistory);

    QMenu* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(helpAction_);
    helpMenu->addAction(aboutAction_);
}

// ---- 无边框自定义标题栏 ----
// 背景：舍弃系统标题栏（Qt::FramelessWindowHint），将"应用图标+标题+拖拽区"、
//       菜单栏、最小化/最大化/关闭按钮组装为一行（VSCode 式紧凑布局）。
// 交互（Windows，nativeEvent 处理）：
//   - 标题栏拖拽区 -> HTCAPTION（系统拖动窗口 + 双击最大化）
//   - 窗口边缘 5px -> HTLEFT/HTTOP/...（边缘调整大小）
//   - WM_GETMINMAXINFO -> 最大化限制到工作区（不遮任务栏）
void MainWindow::createTitleBar() {
    // 左：应用图标 + 标题（内容宽，不抢占空间）
    titleBarWidget_ = new QWidget(this);
    titleBarWidget_->setObjectName(QStringLiteral("titleBar"));
    titleBarWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto* titleLayout = new QHBoxLayout(titleBarWidget_);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(6);

    auto* iconLabel = new QLabel(titleBarWidget_);
    iconLabel->setObjectName(QStringLiteral("titleBarIcon"));
    iconLabel->setPixmap(QPixmap(QStringLiteral(":/perception/icons/icons/png/app/app-icon-24.png")));
    titleLayout->addWidget(iconLabel);

    titleLabel_ = new QLabel(titleBarWidget_);
    titleLabel_->setObjectName(QStringLiteral("titleBarTitle"));
    titleLabel_->setText(windowTitle());
    titleLayout->addWidget(titleLabel_);

    // 右：窗口控制按钮（Windows 惯例：最小化 / 最大化/还原 / 关闭）
    // 图标用 QPainter 统一 16px 矢量绘制（makeWinBtnIcon），颜色随主题 palette；
    // 最大化/还原形状在 updateWindowButtonIcons 中切换。
    constexpr int kBtnW = 40;
    constexpr int kBtnH = 30;
    auto makeWinBtn = [this, kBtnW, kBtnH](const char* name) {
        auto* btn = new QToolButton(this);
        btn->setObjectName(QLatin1String(name));
        btn->setFixedSize(kBtnW, kBtnH);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setIconSize(QSize(16, 16));
        return btn;
    };
    winMinBtn_ = makeWinBtn("winMinBtn");
    winMaxBtn_ = makeWinBtn("winMaxBtn");
    winCloseBtn_ = makeWinBtn("winCloseBtn");
    winMinBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Minimize, palette()));
    winCloseBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Close, palette()));
    winCloseBtn_->setToolTip(tr("关闭"));

    connect(winMinBtn_, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(winMaxBtn_, &QToolButton::clicked, this, &MainWindow::toggleMaximize);
    connect(winCloseBtn_, &QToolButton::clicked, this, &QWidget::close);

    // 组装：标题 + 菜单栏 + 拖拽区 + 窗口按钮 = 同一行（menuWidget 接管顶部）
    // 布局：[图标][标题][文件 编辑 ... 帮助][拖拽区(可拖)][—][□][✕]
    // Windows 传统：菜单在左上角；拖拽区（菜单与按钮之间）支持拖动/双击最大化。
    auto* row = new QWidget(this);
    row->setObjectName(QStringLiteral("titleBarRow"));
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);
    rowLayout->addWidget(titleBarWidget_);
    rowLayout->addWidget(menuBar());  // 菜单栏按内容宽度（不抢拖拽区）

    titleBarDragArea_ = new QWidget(this);
    titleBarDragArea_->setObjectName(QStringLiteral("titleBarDragArea"));
    titleBarDragArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rowLayout->addWidget(titleBarDragArea_);

    rowLayout->addWidget(winMinBtn_);
    rowLayout->addWidget(winMaxBtn_);
    rowLayout->addWidget(winCloseBtn_);
    setMenuWidget(row);

    updateWindowButtonIcons();
}

void MainWindow::toggleMaximize() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

// 003-install-icon-bars：动作图标注册。
// QIcon 在创建时用当时主题的 textDisabled/accent 派生色固化，
// 主题热切换后需按新色板重建，因此统一经 setActionIcon 登记。
void MainWindow::setActionIcon(QAction* action, const QString& iconId) {
    action->setIcon(makeActionIcon(iconId));
    if (!iconItems_.contains({action, iconId})) {
        iconItems_.append({action, iconId});
    }
}

void MainWindow::refreshActionIcons() {
    for (const auto& item : iconItems_) {
        item.first->setIcon(makeActionIcon(item.second));
    }
}

void MainWindow::updateWindowButtonIcons() {
    const bool max = isMaximized();
    winMaxBtn_->setIcon(makeWinBtnIcon(max ? WinBtnKind::Restore : WinBtnKind::Maximize,
                                       palette()));
    winMaxBtn_->setToolTip(max ? tr("还原") : tr("最大化"));
}

// ---- 功能栏（003：左侧通用 + 右侧领域，纵向 ToolButtonIconOnly）----
void MainWindow::createToolbars() {
    // 左：通用功能栏（FR-003：10 按钮 = 5 可用复用菜单动作 + 5 禁用占位，顺序契约 §2）
    leftToolBar_ = new QToolBar(tr("左侧功能栏"), this);
    leftToolBar_->setObjectName(QStringLiteral("leftToolBar"));
    leftToolBar_->setOrientation(Qt::Vertical);
    leftToolBar_->setMovable(false);
    leftToolBar_->setFloatable(false);
    leftToolBar_->setIconSize(QSize(24, 24));
    leftToolBar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    leftToolBar_->setAllowedAreas(Qt::LeftToolBarArea);
    addToolBar(Qt::LeftToolBarArea, leftToolBar_);

    // 契约 id → 既有 QAction 成员映射（FR-004：与菜单同动作，行为 100% 一致）
    const QStringList leftIds = ActionIconMap::leftToolBarOrder();
    for (const QString& id : leftIds) {
        const ActionSpec* spec = ActionIconMap::find(id);
        if (!spec) continue;
        QAction* act = nullptr;
        if (spec->id == QLatin1String("action.openFile"))                act = openAction_;
        else if (spec->id == QLatin1String("action.exportImage"))        act = exportImageAction_;
        else if (spec->id == QLatin1String("action.toggleFileDock"))     act = toggleFileDockAction_;
        else if (spec->id == QLatin1String("action.togglePropertyDock")) act = togglePropertyDockAction_;
        else if (spec->id == QLatin1String("action.togglePythonConsole")) act = togglePythonConsoleAction_;
        else if (spec->id == QLatin1String("action.undo"))               act = undoAction_;
        else if (spec->id == QLatin1String("action.redo"))               act = redoAction_;
        else if (spec->id == QLatin1String("action.loadScript"))         act = loadScriptAction_;
        else if (spec->id == QLatin1String("action.recordScreen"))       act = recordScreenAction_;
        else if (spec->id == QLatin1String("action.refresh"))            act = refreshAction_;
        if (act) {
            act->setToolTip(spec->tooltip);  // FR-006：中文悬停提示
            leftToolBar_->addAction(act);
        }
    }

    // 右：领域功能栏（FR-005：9 按钮，全部禁用态占位，顺序契约 §3）
    rightToolBar_ = new QToolBar(tr("右侧功能栏"), this);
    rightToolBar_->setObjectName(QStringLiteral("rightToolBar"));
    rightToolBar_->setOrientation(Qt::Vertical);
    rightToolBar_->setMovable(false);
    rightToolBar_->setFloatable(false);
    rightToolBar_->setIconSize(QSize(24, 24));
    rightToolBar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    rightToolBar_->setAllowedAreas(Qt::RightToolBarArea);
    addToolBar(Qt::RightToolBarArea, rightToolBar_);

    const QStringList rightIds = ActionIconMap::rightToolBarOrder();
    for (const QString& id : rightIds) {
        const ActionSpec* spec = ActionIconMap::find(id);
        if (!spec) continue;
        // 右栏为独立动作（数据/视图领域功能未实现，全部禁用占位，FR-011）
        QAction* act = new QAction(spec->text, this);
        act->setObjectName(QStringLiteral("rightBar_") + spec->id);
        act->setEnabled(false);
        act->setToolTip(spec->tooltip);  // FR-006：中文悬停提示
        setActionIcon(act, spec->iconId);
        rightToolBar_->addAction(act);
    }
}

// ---- 导出主界面图片（grab + PNG）----
void MainWindow::exportMainWindowImage() {
    // 默认目录跟随程序当前工作目录（= 启动程序时所在路径，FR-015）
    const QString defaultPath = QDir::current().filePath(QStringLiteral("perception.png"));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出主界面图片"), defaultPath, tr("PNG 图片 (*.png)"),
        nullptr, QFileDialog::DontUseNativeDialog);  // 禁原生对话框（ui-guidelines §4.3）
    if (path.isEmpty()) return;

    const QPixmap pm = grab();  // 抓取整个主窗口当前渲染（含菜单/Dock/状态栏）
    if (!pm.save(path, "PNG")) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("无法写入：\n%1").arg(path));
        return;
    }
    statusBar()->showMessage(tr("已导出主界面图片：%1").arg(QFileInfo(path).fileName()), 5000);
}

// ---- 导出控制台命令为 .py 脚本 ----
void MainWindow::exportPythonCommands() {
    const QStringList cmds = pythonConsole_->history();
    if (cmds.isEmpty()) {
        statusBar()->showMessage(tr("没有可导出的命令"), 3000);
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出 Python 命令"),
        QDir::current().filePath(QStringLiteral("console_commands.py")),
        tr("Python 脚本 (*.py)"),
        nullptr, QFileDialog::DontUseNativeDialog);  // 禁原生对话框（ui-guidelines §4.3）
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("无法写入：\n%1").arg(path));
        return;
    }
    QTextStream out(&f);
    out << "# Generated by Perception Python Console (" << cmds.size() << " commands)\n";
    for (const QString& c : cmds) {
        out << c << "\n";
    }
    f.close();
    statusBar()->showMessage(
        tr("已导出 %1 条命令：%2").arg(cmds.size()).arg(QFileInfo(path).fileName()), 5000);
}

// ---- 日志级别（FR-002/012/013 修订）：全局单一矩阵，应用到全部 sink ----
namespace {
// 将指定级别应用到全部已注册 sink（终端/面板/文件），保持全局一致。
// 返回是否有 sink 实际生效。
bool applyLevelToAllSinks(perception::core::log::LogLevel level, bool enabled)
{
    const auto sinks = perception::core::log::Logger::instance().sinks();
    bool applied = false;
    for (const auto& sink : sinks) {
        if (sink) {
            sink->setLevelEnabled(level, enabled);
            applied = true;
        }
    }
    return applied;
}
}  // namespace

void MainWindow::onLevelToggled(bool checked) {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;
    const int idx = act->data().toInt();
    const auto level = static_cast<perception::core::log::LogLevel>(idx);

    // 立即生效：同一级别同步到全部 sink（终端/文件）
    if (!applyLevelToAllSinks(level, checked)) {
        // 之前静默返回——导致用户看到菜单勾上但无输出，以为"设置没用"。
        // 现写入 Logger，让"设置失败"可在日志/终端中看到。
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            std::string("no sink registered; level toggle has no effect (level=")
            + perception::core::log::toString(level) + ")");
    }

    // 持久化（全局 key，FR-013）
    QSettings settings;
    settings.setValue(QString::fromLatin1(kLogLevelPrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                      checked);
}

// 全局级别批量开关：解决"用户找不到单级别怎么开"和"QSettings 残留导致状态混乱"。
void MainWindow::onAllLevels(bool enabled) {
    const auto sinks = perception::core::log::Logger::instance().sinks();
    if (sinks.empty()) {
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            "no sink registered; batch toggle has no effect");
        return;
    }
    QSettings settings;
    for (int i = 0; i < levelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        // setChecked 不会触发 toggled（避免重入），需显式应用 + 持久化
        levelActions_[i]->setChecked(enabled);
        for (const auto& sink : sinks) {
            if (sink) sink->setLevelEnabled(level, enabled);
        }
        settings.setValue(QString::fromLatin1(kLogLevelPrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                          enabled);
    }
}

// 启动恢复：main.cpp 在 Logger::configure + addSink 后调用（FR-013）。
// 读取全局矩阵（log/level/<LEVEL>）；未设置时回退到旧版 log/console/<LEVEL>
//（FR-013 迁移：保证既有用户设置不丢），最后回退默认（DEBUG 关、其余开）。
void MainWindow::restoreLogSettings() {
    QSettings settings;
    for (int i = 0; i < levelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        const QString levelName = QString::fromLatin1(perception::core::log::toString(level));
        const QString key = QString::fromLatin1(kLogLevelPrefix) + levelName;
        const QString legacyKey = QString::fromLatin1(kLogLegacyConsolePrefix) + levelName;
        const bool on = settings.contains(key)
                            ? settings.value(key).toBool()
                            : settings.value(legacyKey, i != 0).toBool();
        levelActions_[i]->setChecked(on);
        for (const auto& sink : perception::core::log::Logger::instance().sinks()) {
            if (sink) sink->setLevelEnabled(level, on);
        }
    }
    if (vtkLogAction_) {
        const bool on = settings.value(QLatin1String(kLogVtkEnabled), true).toBool();
        vtkLogAction_->setChecked(on);
        // VTK 未引入：仅持久化开关；拦截桥随后续落地（FR-011）
    }
}

// ---- 日志路径可达性（FR-014）----
void MainWindow::setLogFilePath(const QString& path) {
    logFilePath_ = path;
    if (logPathAction_)
        logPathAction_->setText(tr("日志文件：%1").arg(path));
    if (openLogDirAction_)
        openLogDirAction_->setEnabled(!path.isEmpty());
    if (setLogPathAction_)
        setLogPathAction_->setEnabled(!path.isEmpty());
    if (clearLogAction_)
        clearLogAction_->setEnabled(!path.isEmpty());
}

void MainWindow::openLogDir() {
    if (logFilePath_.isEmpty()) return;
    const QString dir = QFileInfo(logFilePath_).absolutePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir))) {
        QMessageBox::warning(this, tr("无法打开日志目录"),
                             tr("无法打开目录：%1").arg(dir));
    }
}

// ---- 设置日志路径（FR-016）：选目录 -> 迁移旧日志 -> 重建 FileSink -> 持久化 ----
void MainWindow::setLogPath() {
    if (logFilePath_.isEmpty()) return;
    const QString curDir = QFileInfo(logFilePath_).absolutePath();
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("选择日志保存目录"), curDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    const QString newPath = QDir(dir).filePath(QStringLiteral("app.log"));
    if (newPath == logFilePath_) return;

    const bool ok = perception::core::log::Logger::instance()
                        .setFilePath(newPath.toStdString());
    setLogFilePath(newPath);  // 更新菜单展示与"打开日志目录"/动作可用性
    QSettings settings;
    settings.setValue(QLatin1String(kLogPathKey), newPath);

    PERCEPTION_LOG_I(std::string("log path changed to ") + newPath.toStdString());
    statusBar()->showMessage(
        ok ? tr("日志路径已切换并迁移历史日志：%1").arg(newPath)
           : tr("日志路径已设置（旧日志迁移失败）：%1").arg(newPath), 6000);
}

// ---- 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档 ----
void MainWindow::clearLogHistory() {
    if (logFilePath_.isEmpty()) return;
    const QString dir = QFileInfo(logFilePath_).absolutePath();
    const auto ret = QMessageBox::question(
        this, tr("清除历史日志"),
        tr("将删除日志目录中的全部日志文件（含归档）。\n\n目录：%1\n\n确定继续吗？").arg(dir),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const bool ok = perception::core::log::Logger::instance().clearLogFiles();
    if (ok) {
        PERCEPTION_LOG_I("log history cleared");
        statusBar()->showMessage(tr("已清除历史日志：%1").arg(dir), 5000);
    } else {
        QMessageBox::warning(this, tr("清除失败"),
                             tr("无法清除日志文件，请检查目录权限或磁盘状态。"));
    }
}

// ---- 重置布局（默认布局恢复，供 Ctrl+Shift+L / --snapshot 模式调用）----
void MainWindow::resetLayout() {
    removeDockWidget(fileDock_);
    removeDockWidget(propertyDock_);
    removeDockWidget(pythonDock_);
    QSettings().remove(kSettingsLayoutKey);  // 先清记忆，避免 restoreState 把 dock 再次隐藏
    addDockWidget(Qt::LeftDockWidgetArea, fileDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);
    addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);
    // 强制可见+非浮动（旧 QSettings 状态可能让 dock 隐藏/浮动）
    fileDock_->setVisible(true);
    fileDock_->setFloating(false);
    propertyDock_->setVisible(true);
    propertyDock_->setFloating(false);
    pythonDock_->setVisible(true);
    pythonDock_->setFloating(false);
    pythonDock_->raise();         // 默认激活 Python 控制台
    // 给 dock 一个合理的默认宽度（中央区域给 800+px）
    resizeDocks({fileDock_}, {240}, Qt::Horizontal);
    resizeDocks({propertyDock_}, {280}, Qt::Horizontal);
    resizeDocks({pythonDock_}, {200}, Qt::Vertical);
    statusBar()->showMessage(tr("布局已重置"), 3000);
}

// ---- 主题热切换（菜单触发；--snapshot 截图前切主题再抓图）----
void MainWindow::applyTheme(const QString& themeId) {
    ThemeManager::applyTheme(themeId, *qApp);
    const theme::ThemeDescriptor* t = ThemeManager::current();

    // 勾选状态（含主窗口外的 QSettings 变化，统一对齐）
    for (QAction* act : themeActions_) {
        act->setChecked(act->data().toString() == t->id);
    }
    // 状态栏版本号颜色跟随主题（permanent widget 不随 QSS 自动变色）
    if (versionLabel_) {
        versionLabel_->setStyleSheet(
            QStringLiteral("color: %1;").arg(t->colors.textWeak.name()));
    }
    // 空状态提示文字颜色跟随主题
    updateEmptyHints();
    // Python 控制台提示符/输出配色跟随主题
    if (pythonConsole_) pythonConsole_->refreshColors();
    // 动作图标按新色板重建（QIcon 创建时固化主题色）
    refreshActionIcons();

    statusBar()->showMessage(
        tr("已切换主题：%1").arg(QString::fromUtf8(t->name)), 3000);
}

// ---- Python 运行时释放（main 退出前调用）----
void MainWindow::shutdownPython() {
    if (pythonConsole_) pythonConsole_->shutdown();
}

// ---- Dock 面板 ----
void MainWindow::createDocks() {
    // 左：文件/数据集树（ParaView Pipeline 式，ui-guidelines §5）
    fileDock_ = new QDockWidget(tr("数据集"), this);
    fileDock_->setObjectName(kFileDockObjectName);
    fileDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    // 可分离可恢复：关闭 / 拖动停靠 / 浮动分离（双击标题栏亦可分离与还原）
    fileDock_->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
    fileTree_ = new QTreeWidget(fileDock_);
    fileTree_->setHeaderHidden(true);
    fileTree_->setIndentation(14);
    fileDock_->setWidget(wrapWithSizeGrip(fileTree_, fileDock_));  // 浮动时右下角可调整大小
    fileDock_->setTitleBarWidget(new DockTitleBar(fileDock_));
    addDockWidget(Qt::LeftDockWidgetArea, fileDock_);

    connect(toggleFileDockAction_, &QAction::triggered,
            this, [this](bool on) { fileDock_->setVisible(on); });
    connect(fileDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        toggleFileDockAction_->setChecked(visible);
    });
    toggleFileDockAction_->setChecked(true);

    // 右：属性面板
    propertyDock_ = new QDockWidget(tr("属性"), this);
    propertyDock_->setObjectName(kPropertyDockObjectName);
    propertyDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    propertyDock_->setFeatures(QDockWidget::DockWidgetClosable |
                               QDockWidget::DockWidgetMovable |
                               QDockWidget::DockWidgetFloatable);
    propertyTree_ = new QTreeWidget(propertyDock_);
    propertyTree_->setHeaderHidden(true);
    propertyTree_->setIndentation(14);
    propertyDock_->setWidget(wrapWithSizeGrip(propertyTree_, propertyDock_));
    propertyDock_->setTitleBarWidget(new DockTitleBar(propertyDock_));
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

    connect(togglePropertyDockAction_, &QAction::triggered,
            this, [this](bool on) { propertyDock_->setVisible(on); });
    connect(propertyDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        togglePropertyDockAction_->setChecked(visible);
    });
    togglePropertyDockAction_->setChecked(true);

    // 底：Python 控制台（内嵌 CPython REPL，M5 命令层的前身）
    pythonDock_ = new QDockWidget(tr("Python 控制台"), this);
    pythonDock_->setObjectName(kPythonDockObjectName);
    pythonDock_->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    pythonDock_->setFeatures(QDockWidget::DockWidgetClosable |
                             QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable);
    // 容器：左侧控制台 + 右侧操作按钮栏（导出命令 / 清空控制台）
    auto* pythonContainer = new QWidget(pythonDock_);
    pythonContainer->setObjectName(QStringLiteral("pythonConsoleContainer"));
    auto* pythonLayout = new QHBoxLayout(pythonContainer);
    pythonLayout->setContentsMargins(0, 0, 0, 0);
    pythonLayout->setSpacing(0);

    pythonConsole_ = new PythonConsole(pythonContainer);
    pythonLayout->addWidget(pythonConsole_, 1);

    auto* sideBar = new QWidget(pythonContainer);
    sideBar->setObjectName(QStringLiteral("pythonSideBar"));
    auto* sideLayout = new QVBoxLayout(sideBar);
    sideLayout->setContentsMargins(6, 8, 6, 8);
    sideLayout->setSpacing(6);

    auto* exportBtn = new QToolButton(sideBar);
    exportBtn->setObjectName(QStringLiteral("pythonExportBtn"));
    exportBtn->setText(tr("导出命令"));
    exportBtn->setToolTip(tr("将已执行的 Python 命令导出为 .py 脚本"));
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setFixedSize(64, 28);
    connect(exportBtn, &QToolButton::clicked,
            this, &MainWindow::exportPythonCommands);

    auto* clearBtn = new QToolButton(sideBar);
    clearBtn->setObjectName(QStringLiteral("pythonClearBtn"));
    clearBtn->setText(tr("清空控制台"));
    clearBtn->setToolTip(tr("清空控制台显示内容（保留已定义变量）"));
    clearBtn->setCursor(Qt::PointingHandCursor);
    clearBtn->setFixedSize(64, 28);
    connect(clearBtn, &QToolButton::clicked,
            this, [this] { pythonConsole_->clearConsole(); });

    sideLayout->addWidget(exportBtn);
    sideLayout->addStretch();
    sideLayout->addWidget(clearBtn);
    pythonLayout->addWidget(sideBar);

    pythonDock_->setWidget(wrapWithSizeGrip(pythonContainer, pythonDock_));
    pythonDock_->setTitleBarWidget(new DockTitleBar(pythonDock_));
    addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);

    // 抑制 dock 获得键盘焦点时的 focus rect 边框（避免点击 dock 内容时出现一圈高亮）
    applyNoFocusRectStyle(fileDock_);
    applyNoFocusRectStyle(propertyDock_);
    applyNoFocusRectStyle(pythonDock_);
    connect(togglePythonConsoleAction_, &QAction::triggered,
            this, [this](bool on) { pythonDock_->setVisible(on); });
    connect(pythonDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        togglePythonConsoleAction_->setChecked(visible);
    });
    togglePythonConsoleAction_->setChecked(true);

    // 重置布局
    connect(resetLayoutAction_, &QAction::triggered, this, &MainWindow::resetLayout);
}

// ---- Dock 拖拽高亮（VSCode 风格：拖拽面板时目标分割线高亮）----
// 由 DockTitleBar::eventFilter 驱动：press→Candidate→(超过阈值)→Dragging，
// beginDockDrag 显示覆盖层，updateDockDrag 按鼠标位置更新高亮目标，
// endDockDrag 执行放置（addDockWidget 移动 dock 到目标区域）。
void MainWindow::beginDockDrag(QDockWidget* dock) {
    dragDock_ = dock;
    if (!dockDragOverlay_) {
        dockDragOverlay_ = new DockDragOverlay(this);
        dockDragOverlay_->setGeometry(rect());
    }
    dockDragOverlay_->raise();
    dockDragOverlay_->show();
}

// ---- 分界线（dock 分隔条）resize 拖拽高亮 ----
// 由 MainWindow::event 检测分隔条命中驱动（不吞事件，resize 仍由 QMainWindowLayout 完成）。
// 高亮条 = SashHighlight 细条 widget（仅覆盖分隔条缝隙，亮色，局部重绘，拖动不卡顿）；
// 位置在 QMainWindowLayout 处理完 move（布局更新）后同步 → 与真实分隔条完全重合。
void MainWindow::beginSashDrag(int hit) {
    sashDragging_ = true;
    sashHit_ = hit;
    if (!sashHighlight_) sashHighlight_ = new SashHighlight(this);
    sashHighlight_->raise();
    sashHighlight_->show();
    updateSashDrag();  // 初始位置 = 当前分隔条（无拖拽，准确）
}

void MainWindow::updateSashDrag() {
    if (!sashDragging_ || !sashHighlight_) return;
    const QRect line = sashHighlightRect();  // 最新分隔条 rect（布局已更新）
    if (line.isEmpty()) { sashHighlight_->hide(); return; }
    // overlay 精确覆盖分隔条缝隙（4px 粗）：仅一个 widget 的局部重绘，代价极小
    sashHighlight_->setGeometry(line);
}

void MainWindow::endSashDrag() {
    if (!sashDragging_) return;
    sashDragging_ = false;
    sashHit_ = SashMiss;
    if (sashHighlight_) sashHighlight_->hide();
}

// 分隔条矩形：由命中类型 + 最新 dock geometry 计算。长度 = 分隔条实际长度
// （水平分隔条 = pythonDock 宽度；垂直分隔条 = 对应 dock 高度），与真实分隔条 100% 吻合。
QRect MainWindow::sashHighlightRect() const {
    switch (sashHit_) {
    case SashFileRight:
        if (fileDock_ && fileDock_->isVisible() && !fileDock_->isFloating()) {
            const QRect g = fileDock_->geometry();
            // 垂直分隔条在 fileDock 右缘 +1px 处；4px 条中心对齐
            return QRect(g.right() - 1, g.top(), 4, g.height());
        }
        break;
    case SashPropertyLeft:
        if (propertyDock_ && propertyDock_->isVisible() && !propertyDock_->isFloating()) {
            const QRect g = propertyDock_->geometry();
            // 垂直分隔条在 propertyDock 左缘 -1px 处
            return QRect(g.left() - 3, g.top(), 4, g.height());
        }
        break;
    case SashPythonTop:
        if (pythonDock_ && pythonDock_->isVisible() && !pythonDock_->isFloating()) {
            const QRect g = pythonDock_->geometry();
            // 水平分隔条在 pythonDock 上缘 -1px 处；4px 条覆盖其上
            return QRect(g.left(), g.top() - 2, g.width(), 4);
        }
        break;
    default:
        break;
    }
    return QRect();
}

void MainWindow::updateDockDrag(const QPoint& globalPos) {
    if (!dockDragOverlay_ || !dragDock_) return;
    const QPoint pos = mapFromGlobal(globalPos);
    const QRect r = rect();
    const int zoneW = qRound(r.width() * 0.22);   // 左右放置带宽度（VSCode 式分带）
    const int zoneH = qRound(r.height() * 0.22);  // 底部放置带高度

    // 鼠标在哪个放置带？VSCode 行为：dock 标题栏被拖动时，分割线立即亮。
    // 鼠标在 dock 内部（不在任何 zone）时，默认高亮 dock 当前所在侧的分割线——
    // 这样用户拖动 fileDock 标题栏时 fileDock 右缘分割线立刻有高亮，
    // 把鼠标移到右/底 zone 时高亮位置随之切换。
    enum class Zone { Left, Right, Bottom };
    auto pickZone = [&]() {
        if (pos.x() < zoneW)        return Zone::Left;
        if (pos.x() >= r.width() - zoneW) return Zone::Right;
        if (pos.y() >= r.height() - zoneH) return Zone::Bottom;
        // 鼠标在 dock 内部：保持 dock 当前所在区域
        const auto a = dockWidgetArea(dragDock_);
        if (a == Qt::RightDockWidgetArea)  return Zone::Right;
        if (a == Qt::BottomDockWidgetArea) return Zone::Bottom;
        return Zone::Left;  // 默认/左
    };
    const Zone zone = pickZone();

    QRect fill;  // 目标区域半透明填充
    QRect line;  // 分割线高亮条（3px）
    if (zone == Zone::Left) {
        const int x = (fileDock_ && fileDock_->isVisible())
                          ? fileDock_->geometry().right()
                          : qRound(r.width() * 0.22);
        fill = QRect(0, 0, x, r.height());
        line = QRect(x - 1, 0, 3, r.height());
    } else if (zone == Zone::Right) {
        const int x = (propertyDock_ && propertyDock_->isVisible())
                          ? propertyDock_->geometry().left()
                          : qRound(r.width() * 0.78);
        fill = QRect(x, 0, r.width() - x, r.height());
        line = QRect(x - 1, 0, 3, r.height());
    } else {
        const int y = (pythonDock_ && pythonDock_->isVisible())
                          ? pythonDock_->geometry().top()
                          : qRound(r.height() * 0.78);
        fill = QRect(0, y, r.width(), r.height() - y);
        line = QRect(0, y - 1, r.width(), 3);
    }
    static_cast<DockDragOverlay*>(dockDragOverlay_)->setHighlight(fill, line);
}

void MainWindow::endDockDrag(const QPoint& globalPos) {
    dockDragOverlay_->hide();
    if (!dragDock_) return;
    const QPoint pos = mapFromGlobal(globalPos);
    const int zoneW = qRound(width() * 0.22);
    const int zoneH = qRound(height() * 0.22);

    Qt::DockWidgetArea area = Qt::NoDockWidgetArea;
    if (pos.x() < zoneW) {
        area = Qt::LeftDockWidgetArea;
    } else if (pos.x() >= width() - zoneW) {
        area = Qt::RightDockWidgetArea;
    } else if (pos.y() >= height() - zoneH) {
        area = Qt::BottomDockWidgetArea;
    }
    // 目标区域允许停靠且与当前位置不同才移动（allowedAreas 限制：如左/右面板不可到底部）
    if (area != Qt::NoDockWidgetArea
        && (dragDock_->allowedAreas() & area)
        && dockWidgetArea(dragDock_) != area) {
        addDockWidget(area, dragDock_);
    }
    dragDock_ = nullptr;
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (dockDragOverlay_) dockDragOverlay_->setGeometry(rect());
}

// ---- 中央区域（空状态设计，ui-guidelines §5.1）----
void MainWindow::createCentralArea() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    centralPlaceholder_ = new QLabel(central);
    centralPlaceholder_->setObjectName("centralPlaceholder");
    centralPlaceholder_->setAlignment(Qt::AlignCenter);
    centralPlaceholder_->setText(tr("拖放 .plt / .csv 文件到此处\n或按 Ctrl+O 打开数据文件"));
    layout->addWidget(centralPlaceholder_);

    setCentralWidget(central);
}

// ---- 状态栏 ----
void MainWindow::createStatusBar() {
    QStatusBar* sb = statusBar();
    sb->showMessage(tr("就绪"));

    versionLabel_ = new QLabel(tr("v%1").arg(QApplication::applicationVersion()), sb);
    const QColor weak = ThemeManager::current()->colors.textWeak;
    versionLabel_->setStyleSheet(QStringLiteral("color: %1;").arg(weak.name()));
    sb->addPermanentWidget(versionLabel_);
}

// ---- 空状态提示 ----
void MainWindow::updateEmptyHints() {
    const QColor weak = ThemeManager::current()->colors.textWeak;
    if (fileTree_->topLevelItemCount() == 0) {
        auto* item = new QTreeWidgetItem(fileTree_, {QString::fromUtf8(kEmptyTreeText)});
        item->setForeground(0, weak);
        item->setFlags(Qt::NoItemFlags);
    }
    if (propertyTree_->topLevelItemCount() == 0) {
        auto* item = new QTreeWidgetItem(propertyTree_, {QString::fromUtf8(kEmptyPropertyText)});
        item->setForeground(0, weak);
        item->setFlags(Qt::NoItemFlags);
    }
}

// ---- 打开文件 ----
void MainWindow::openFile() {
    const QString file = QFileDialog::getOpenFileName(
        this, tr("打开数据文件"), QDir::currentPath(),
        tr("曲线数据 (*.plt *.csv);;所有文件 (*)"),
        nullptr, QFileDialog::DontUseNativeDialog);  // 禁原生对话框（ui-guidelines §4.3）
    if (file.isEmpty()) return;

    addFileToTree(file);
    statusBar()->showMessage(tr("已加载：%1").arg(QFileInfo(file).fileName()), 5000);
}

// ---- 加入文件树（占位：M2 数据层接入后替换为真实加载）----
void MainWindow::addFileToTree(const QString& path) {
    auto* root = fileTree_->invisibleRootItem();
    // 移除空提示节点
    for (int i = root->childCount() - 1; i >= 0; --i) {
        if (root->child(i)->flags() == Qt::NoItemFlags) {
            delete root->takeChild(i);
        }
    }
    const QString name = QFileInfo(path).fileName();
    auto* item = new QTreeWidgetItem(root, {name});
    item->setData(0, Qt::UserRole, path);
    item->setToolTip(0, path);
    fileTree_->expandAll();
}

// ---- 拖放 ----
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    int added = 0;
    for (const QUrl& url : urls) {
        if (!url.isLocalFile()) continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            addFileToTree(path);
            ++added;
        }
    }
    if (added > 0) {
        statusBar()->showMessage(tr("已加载 %1 个文件").arg(added), 5000);
    }
    event->acceptProposedAction();
}

// ---- 布局记忆（ui-guidelines §5.1）----
void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue(kSettingsGeometryKey, saveGeometry());
    settings.setValue(kSettingsLayoutKey, saveState());
    QMainWindow::closeEvent(event);
}

// ---- 关于 ----
void MainWindow::about() {
    QMessageBox::about(this, tr("关于 Perception"),
        tr("<h3>Perception %1</h3>"
           "<p>数据可视化桌面工具（对标 ParaView / SVisual）。</p>"
           "<p>技术栈：C++17 / Qt / VTK / pybind11</p>")
            .arg(QApplication::applicationVersion()));
}

// ---- 无边框窗口状态同步 ----
void MainWindow::changeEvent(QEvent* e) {
    if (e->type() == QEvent::WindowStateChange) {
        // 系统/外部触发最大化或还原（如任务栏右键菜单）时同步按钮图标
        updateWindowButtonIcons();
    }
    QMainWindow::changeEvent(e);
}

bool MainWindow::event(QEvent* e) {
    // 主题切换（ThemeManager 更新 palette）后，标题栏按钮图标颜色随之刷新
    if ((e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) &&
        winMinBtn_ != nullptr) {
        winMinBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Minimize, palette()));
        winCloseBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Close, palette()));
        updateWindowButtonIcons();
    }

    // 分界线（dock 分隔条）resize 拖拽高亮：
    // 1) press 在转发给 QMainWindowLayout 之前检测命中——此时无拖拽、dock geometry 准确，
    //    天然区分左右/上下多条分隔条（无旧方案"拖右亮左"歧义）；命中后不吞事件，Qt 正常拖拽。
    // 2) release 在转发前结束高亮。
    // 3) move 在转发后同步高亮条——QMainWindowLayout 已处理 separatorMove、布局已更新，
    //    高亮条位置与真实分隔条完全重合（跟手）。
    if (e->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {
            const int hit = sashHitTest(me->pos());
            if (hit) beginSashDrag(hit);
        }
    } else if (e->type() == QEvent::MouseButtonRelease && sashDragging_) {
        endSashDrag();
    }

    const bool handled = QMainWindow::event(e);

    if (sashDragging_ && e->type() == QEvent::MouseMove) {
        updateSashDrag();  // 转发后布局已更新 → 高亮条与真实分隔条同步
    }
    return handled;
}

// ---- 无边框窗口原生消息（Windows）----
// 核心：无边框窗口失去系统标题栏后，拖拽/双击最大化/边缘 resize 全部由系统
// 消息驱动。这里只做两件事：
//   1. WM_NCHITTEST 自定义命中检测（标题栏拖拽区 -> HTCAPTION，边缘 -> HT*）
//   2. WM_GETMINMAXINFO 限制最大化尺寸到工作区（不遮任务栏）
// 菜单栏与窗口按钮区域返回 HTCLIENT，保证点击/弹出菜单正常工作。
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result) {
#ifdef Q_OS_WIN
    auto* msg = static_cast<MSG*>(message);
    switch (msg->message) {
    case WM_NCHITTEST: {
        *result = 0;
        const QPoint global(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        const QPoint local = mapFromGlobal(global);
        const int w = width();
        const int h = height();
        constexpr int kEdge = 5;  // 边缘 resize 热区宽度（px）

        // 最大化/全屏时不提供边缘热区（窗口已铺满工作区）
        if (!isMaximized() && !isFullScreen()) {
            const bool left = local.x() < kEdge;
            const bool right = local.x() >= w - kEdge;
            const bool top = local.y() < kEdge;
            const bool bottom = local.y() >= h - kEdge;
            if (left && top) { *result = HTTOPLEFT; return true; }
            if (right && top) { *result = HTTOPRIGHT; return true; }
            if (left && bottom) { *result = HTBOTTOMLEFT; return true; }
            if (right && bottom) { *result = HTBOTTOMRIGHT; return true; }
            if (left) { *result = HTLEFT; return true; }
            if (right) { *result = HTRIGHT; return true; }
            if (top) { *result = HTTOP; return true; }
            if (bottom) { *result = HTBOTTOM; return true; }
        }

        // 标题栏/拖拽区 -> HTCAPTION：系统接管拖动（含 Aero 吸附）与双击最大化
        QWidget* hit = childAt(local);
        for (QWidget* p = hit; p && p != this; p = p->parentWidget()) {
            if (p == titleBarWidget_ || p == titleBarDragArea_) {
                *result = HTCAPTION;
                return true;
            }
        }
        *result = HTCLIENT;  // 其余区域（菜单栏/按钮/Dock/中央）交还 Qt 处理
        return true;
    }
    case WM_NCLBUTTONDBLCLK:
        if (msg->wParam == HTCAPTION) {
            toggleMaximize();  // 双击标题栏：最大化/还原
            return true;
        }
        break;
    case WM_GETMINMAXINFO: {
        // 无边框最大化：限制到工作区，避免覆盖任务栏
        auto* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        if (const QScreen* scr = screen()) {
            const QRect avail = scr->availableGeometry();
            mmi->ptMaxPosition.x = avail.x();
            mmi->ptMaxPosition.y = avail.y();
            mmi->ptMaxSize.x = avail.width();
            mmi->ptMaxSize.y = avail.height();
        }
        *result = 0;
        return true;
    }
    default:
        break;
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

}  // namespace ui
}  // namespace perception

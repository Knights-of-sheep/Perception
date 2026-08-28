// ===== Perception 主窗口实现（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// 规范：docs/design/ui-guidelines.md §5 布局范式 / §6 交互规范。
#include "ui/MainWindow.h"
#include "ui/win_btn_icon.h"
#include "ui/window_geometry.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
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
#include "ui/subwindow/layout_settings_dialog.h"
#include "ui/subwindow/subwindow_container.h"
#include "ui/subwindow/subwindow_view.h"
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
constexpr const char* kEmptyTreeText          = "(no files loaded)";
constexpr const char* kEmptyPropertyText      = "(select an object to view properties)";

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

// 窗口控制按钮图标见 ui/win_btn_icon.h（统一 16px 矢量绘制；
// 文本字符字形/基线不一观感不齐，故全部用 QPainter 图标）。

// ---- 通用对话框标题栏工厂（FramelessDialog 与 ThemedFileDialog 共享） ----
// 复用 QSS objectName (titleBarRow/titleBarTitle/titleBarIcon/winCloseBtn) 与
// makeWinBtnIcon 图标，保证所有无边框窗口标题栏外观一致、随主题。
// owner: 用于 parent 与 close 事件连接（QFileDialog/FramelessDialog 均为 QDialog 子类）。
QWidget* buildDialogTitleBar(QDialog* owner, const QString& title) {
    auto* bar = new QWidget(owner);
    bar->setObjectName(QStringLiteral("titleBarRow"));
    bar->setFixedHeight(30);
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(6);

    auto* icon = new QLabel(bar);
    icon->setObjectName(QStringLiteral("titleBarIcon"));
    icon->setPixmap(QPixmap(QStringLiteral(":/perception/icons/icons/png/app/app-icon-24.png")));
    layout->addWidget(icon);

    auto* titleLabel = new QLabel(bar);
    titleLabel->setObjectName(QStringLiteral("titleBarTitle"));
    titleLabel->setText(title);
    layout->addWidget(titleLabel);
    layout->addStretch();

    auto* closeBtn = new QToolButton(bar);
    closeBtn->setObjectName(QStringLiteral("winCloseBtn"));
    closeBtn->setFixedSize(40, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setAutoRaise(true);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setIconSize(QSize(16, 16));
    closeBtn->setIcon(makeWinBtnIcon(WinBtnKind::Close, bar->palette()));
    closeBtn->setToolTip(QObject::tr("Close"));
    QObject::connect(closeBtn, &QToolButton::clicked, owner, [owner] { owner->close(); });
    layout->addWidget(closeBtn);

    return bar;
}

// ---- 文件/目录对话框：无边框容器 + 自定义标题栏 + 内嵌 QFileDialog ----
// 背景：QFileDialog::getOpenFileName 等静态接口即使 setOption(DontUseNativeDialog)
//   也是系统标题栏，且 Qt 5.15 公开 API 无 setTitleBarWidget（仅 QDockWidget 有）。
//   而 ui-guidelines §4.3 又要求禁原生对话框以贴合 Qt 风格。
// 方案：Qt::FramelessWindowHint 的 QDialog 包裹 QFileDialog（Qt::Widget 化嵌入）：
//   自定义标题栏走 buildDialogTitleBar；QFileDialog 的 OK/Cancel 信号映射到 accept/reject。
enum class FileDialogMode { Open, Save, Directory };

class ThemedFileDialog : public QDialog {
public:
    ThemedFileDialog(QWidget* parent, const QString& title,
                     const QString& dir, const QString& filter,
                     FileDialogMode mode)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
        setWindowTitle(title);
        setWindowIcon(QApplication::windowIcon());

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // 标题栏：复用主界面 titleBarRow 样式
        titleBar_ = buildDialogTitleBar(this, title);
        root->addWidget(titleBar_);

        // QFileDialog 作为普通 widget 嵌入（Qt::Widget 剥除其窗口属性）
        fileDialog_ = new QFileDialog(this, title, dir, filter);
        fileDialog_->setOption(QFileDialog::DontUseNativeDialog, true);
        fileDialog_->setWindowFlags(Qt::Widget);
        fileDialog_->setSizeGripEnabled(false);
        if (mode == FileDialogMode::Directory) {
            fileDialog_->setFileMode(QFileDialog::Directory);
            fileDialog_->setOption(QFileDialog::ShowDirsOnly, true);
            fileDialog_->setOption(QFileDialog::DontResolveSymlinks, true);
        } else if (mode == FileDialogMode::Save) {
            fileDialog_->setAcceptMode(QFileDialog::AcceptSave);
            fileDialog_->setFileMode(QFileDialog::AnyFile);
        } else {
            fileDialog_->setAcceptMode(QFileDialog::AcceptOpen);
            fileDialog_->setFileMode(QFileDialog::AnyFile);
        }
        connect(fileDialog_, &QFileDialog::accepted, this, &QDialog::accept);
        connect(fileDialog_, &QFileDialog::rejected, this, &QDialog::reject);

        root->addWidget(fileDialog_);

        setMinimumSize(640, 480);
        adjustSize();
    }

    QString selectedFile() const {
        return fileDialog_->selectedFiles().value(0);
    }

protected:
    // 标题栏拖拽移动
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && e->pos().y() <= titleBar_->height()) {
            dragOffset_ = e->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if ((e->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
            move(e->globalPos() - dragOffset_);
        }
        QDialog::mouseMoveEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        dragOffset_ = QPoint();
        QDialog::mouseReleaseEvent(e);
    }

private:
    QWidget* titleBar_ = nullptr;
    QFileDialog* fileDialog_ = nullptr;
    QPoint dragOffset_;
};

// 文件/目录对话框统一入口（Open / Save / Directory）
QString runThemedFileDialog(QWidget* parent, const QString& title,
                            const QString& dir, const QString& filter,
                            FileDialogMode mode) {
    ThemedFileDialog dlg(parent, title, dir, filter, mode);
    return (dlg.exec() == QDialog::Accepted) ? dlg.selectedFile() : QString();
}

// ---- 帮助/关于弹窗：无边框 + 自定义标题栏 ----
// 背景：QMessageBox 是系统原生标题栏，与主界面/Dock 自定义标题栏风格割裂。
// 方案：Qt::FramelessWindowHint 的 QDialog，标题栏复用 buildDialogTitleBar，
//       正文富文本 + 确定按钮。
class FramelessDialog : public QDialog {
public:
    FramelessDialog(QWidget* parent, const QString& title, const QString& html)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);
        setWindowTitle(title);
        setWindowIcon(QApplication::windowIcon());

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // 标题栏：复用主界面 titleBarRow 样式（@panelBg@ + 底部分隔线）
        titleBar_ = buildDialogTitleBar(this, title);
        root->addWidget(titleBar_);

        // 内容区：富文本正文 + 确定按钮（QSS 默认态 → primary accent）
        auto* body = new QVBoxLayout();
        body->setContentsMargins(20, 18, 20, 16);
        body->setSpacing(16);

        auto* content = new QLabel(this);
        content->setTextFormat(Qt::RichText);
        content->setWordWrap(true);
        content->setText(html);
        body->addWidget(content);

        auto* okRow = new QHBoxLayout();
        okRow->addStretch();
        auto* okBtn = new QPushButton(tr("OK"), this);
        okBtn->setDefault(true);
        connect(okBtn, &QPushButton::clicked, this, &QDialog::close);
        okRow->addWidget(okBtn);
        body->addLayout(okRow);

        root->addLayout(body);

        setMinimumWidth(400);
        adjustSize();
    }

protected:
    // 标题栏拖拽移动（去系统标题栏后自行处理）
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && e->pos().y() <= titleBar_->height()) {
            dragOffset_ = e->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if ((e->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
            move(e->globalPos() - dragOffset_);
        }
        QDialog::mouseMoveEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        dragOffset_ = QPoint();
        QDialog::mouseReleaseEvent(e);
    }

private:
    QWidget* titleBar_ = nullptr;
    QPoint dragOffset_;
};

// 帮助/关于统一入口：模态显示无边框对话框
void showFramelessDialog(QWidget* parent, const QString& title, const QString& html) {
    auto* dlg = new FramelessDialog(parent, title, html);
    dlg->exec();
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
        floatBtn_->setToolTip(floating ? tr("Re-dock") : tr("Undock"));
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
        maxBtn_->setToolTip(max ? tr("Restore") : tr("Maximize"));
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

    // 004-dock-layout-manager：Python create_window 桥 → 创建子窗口（FR-001，契约
    // contracts/python-create-window.md；REPL 在 GUI 线程执行，直连安全）
    connect(pythonConsole_, &PythonConsole::createWindowRequested, this,
            &MainWindow::createSubwindow);

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
    openAction_ = new QAction(tr("&Open..."), this);
    openAction_->setShortcut(QKeySequence::Open);  // Ctrl+O
    openAction_->setStatusTip(tr("Open VTK / SVisual / HDF5 data files"));
    setActionIcon(openAction_, "file-open");  // 003：契约图标（下同）
    connect(openAction_, &QAction::triggered, this, &MainWindow::openFile);

    exportAction_ = new QAction(tr("Export Command Script(&E)..."), this);
    exportAction_->setShortcut(QKeySequence::SaveAs);  // Ctrl+Shift+S
    exportAction_->setEnabled(false);  // 契约 §1：禁用态保持（导出命令脚本未实现）
    setActionIcon(exportAction_, "file-export-data");
    connect(exportAction_, &QAction::triggered, this, [this] {
        statusBar()->showMessage(tr("Export command script will be available in a future release"), 3000);
    });

    // 导出主界面图片（与"打开"同级，文件菜单；M3a 截图调试验证用）
    exportImageAction_ = new QAction(tr("Export Main Window Image(&I)..."), this);
    exportImageAction_->setShortcut(QKeySequence("Ctrl+I"));
    exportImageAction_->setStatusTip(tr("Save the current main window (menus/docks/status bar) as a PNG image"));
    setActionIcon(exportImageAction_, "file-export-screenshot");
    connect(exportImageAction_, &QAction::triggered, this, &MainWindow::exportMainWindowImage);

    exitAction_ = new QAction(tr("E&xit"), this);
    exitAction_->setShortcut(QKeySequence::Quit);
    setActionIcon(exitAction_, "file-close");
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    toggleFileDockAction_ = new QAction(tr("Data Panel"), this);
    toggleFileDockAction_->setCheckable(true);
    toggleFileDockAction_->setShortcut(QKeySequence("Ctrl+1"));
    setActionIcon(toggleFileDockAction_, "view-panel-data");

    togglePropertyDockAction_ = new QAction(tr("Properties Panel"), this);
    togglePropertyDockAction_->setCheckable(true);
    togglePropertyDockAction_->setShortcut(QKeySequence("Ctrl+2"));
    setActionIcon(togglePropertyDockAction_, "view-panel-property");

    togglePythonConsoleAction_ = new QAction(tr("Python Console"), this);
    togglePythonConsoleAction_->setCheckable(true);
    togglePythonConsoleAction_->setShortcut(QKeySequence("Ctrl+`"));
    setActionIcon(togglePythonConsoleAction_, "view-panel-console");

    resetLayoutAction_ = new QAction(tr("Reset Layout"), this);
    resetLayoutAction_->setShortcut(QKeySequence("Ctrl+Shift+L"));
    resetLayoutAction_->setStatusTip(tr("Restore the default layout (data left, properties right)"));
    setActionIcon(resetLayoutAction_, "view-reset-camera");

    // 004-dock-layout-manager：创建子窗口与布局设置（FR-001/002，US5 统一入口）
    newSubwindowAction_ = new QAction(tr("New Subwindow"), this);
    newSubwindowAction_->setStatusTip(tr("Create a new render subwindow"));
    setActionIcon(newSubwindowAction_, "view-multi-view");
    connect(newSubwindowAction_, &QAction::triggered, this,
            [this] { createSubwindow(tr("Untitled")); });

    layoutSettingsAction_ = new QAction(tr("Layout Settings..."), this);
    layoutSettingsAction_->setStatusTip(tr("Arrange subwindows: mode, max rows/columns, same size"));
    setActionIcon(layoutSettingsAction_, "view-multi-view");
    connect(layoutSettingsAction_, &QAction::triggered, this, &MainWindow::openLayoutSettings);

    // 004：全屏 = 中间区域（子窗口容器）扩展至整个主界面（隐藏 Dock；FR-017）。
    // View 菜单入口；checkable 表示当前处于全屏（侧边栏无此按钮，见 createToolbars）。
    toggleFullscreenAction_ = new QAction(tr("Fullscreen"), this);
    toggleFullscreenAction_->setCheckable(true);
    toggleFullscreenAction_->setStatusTip(
        tr("Expand the central area to the whole main window (hide docks)"));
    setActionIcon(toggleFullscreenAction_, "view-fit-screen");
    connect(toggleFullscreenAction_, &QAction::triggered, this,
            [this] { setContainerFullscreen(!containerFullscreen_); });

    // 004：恢复被"隐藏"的子窗口（View 菜单；隐藏按钮在子窗口标题栏）
    showHiddenSubwindowsAction_ = new QAction(tr("Show Hidden Subwindows"), this);
    showHiddenSubwindowsAction_->setStatusTip(tr("Restore subwindows hidden via the title-bar button"));
    connect(showHiddenSubwindowsAction_, &QAction::triggered, this,
            &MainWindow::showHiddenSubwindows);

    // 003：未实现功能占位动作（FR-011：禁用态 + tooltip 明确提示，不连接功能槽）
    undoAction_ = new QAction(tr("&Undo"), this);
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);
    undoAction_->setStatusTip(tr("Undo the last action (coming soon)"));
    setActionIcon(undoAction_, "edit-undo");

    redoAction_ = new QAction(tr("&Redo"), this);
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false);
    redoAction_->setStatusTip(tr("Redo the undone action (coming soon)"));
    setActionIcon(redoAction_, "edit-redo");

    loadScriptAction_ = new QAction(tr("Load Script(&L)..."), this);
    loadScriptAction_->setEnabled(false);
    loadScriptAction_->setStatusTip(tr("Load a Python script (coming soon)"));
    setActionIcon(loadScriptAction_, "file-load-script");

    recordScreenAction_ = new QAction(tr("Record Main Window Video(&V)"), this);
    recordScreenAction_->setEnabled(false);
    recordScreenAction_->setStatusTip(tr("Record the main window as a video (coming soon)"));
    setActionIcon(recordScreenAction_, "file-record-screen");

    refreshAction_ = new QAction(tr("Refresh(&F)"), this);
    refreshAction_->setEnabled(false);
    refreshAction_->setStatusTip(tr("Refresh the current view (coming soon)"));
    setActionIcon(refreshAction_, "view-refresh");

    helpAction_ = new QAction(tr("&Help"), this);
    helpAction_->setStatusTip(tr("View help documentation"));
    setActionIcon(helpAction_, "tools-help");
    connect(helpAction_, &QAction::triggered, this, [this] {
        showFramelessDialog(this, tr("Help"), tr("Help documentation will be available in a future release"));
    });

    aboutAction_ = new QAction(tr("&About..."), this);
    setActionIcon(aboutAction_, "tools-about");
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::about);
}

// ---- 菜单栏 ----
void MainWindow::createMenus() {
    // 禁用 Windows 原生菜单栏：否则菜单栏由系统绘制（浅色、不走 QSS），
    // 与深色主题割裂，窗口顶部会呈现"两层"的观感。
    menuBar()->setNativeMenuBar(false);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction_);
    fileMenu->addAction(exportAction_);  // 导出命令脚本（禁用态保持，契约 §1）
    fileMenu->addAction(exportImageAction_);  // 导出主界面图片（与"打开"同级，调试快照）
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction_);

    // 编辑（003：撤销/重做占位，FR-011 禁用态）
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAction_);
    editMenu->addAction(redoAction_);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(toggleFileDockAction_);
    viewMenu->addAction(togglePropertyDockAction_);
    viewMenu->addAction(togglePythonConsoleAction_);
    viewMenu->addSeparator();
    viewMenu->addAction(newSubwindowAction_);      // 004：新建子窗口（FR-001/002）
    viewMenu->addAction(layoutSettingsAction_);    // 004：布局设置（US5 统一入口）
    viewMenu->addAction(toggleFullscreenAction_);  // 004：全屏（中央区域扩展占满主界面，FR-017）
    viewMenu->addAction(showHiddenSubwindowsAction_);  // 004：恢复隐藏子窗口
    viewMenu->addSeparator();
    viewMenu->addAction(resetLayoutAction_);

    // 主题（25 套，按 family 分组；勾选当前项，点击即热切换）
    QMenu* themeMenu = menuBar()->addMenu(tr("&Theme"));
    themeGroup_ = new QActionGroup(this);
    themeGroup_->setExclusive(true);
    QString currentFamily;
    const auto* themes = ThemeManager::themes();
    for (int i = 0; i < ThemeManager::themeCount(); ++i) {
        const auto& t = themes[i];
        const QString family = QString::fromUtf8(t.family);
        if (i > 0 && family != currentFamily) {
            themeMenu->addSeparator();  // 深色 / 浅色 / 高对比 分组
        }
        currentFamily = family;

        QAction* act = themeMenu->addAction(QString::fromUtf8(t.name));
        act->setCheckable(true);
        act->setData(QString::fromLatin1(t.id));
        act->setStatusTip(tr("Switch to theme: %1 (%2)")
                              .arg(QString::fromUtf8(t.name), family));
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
    // 全局单一矩阵：级别同时作用于全部 sink（终端/文件），不再区分控制台/文件（FR-012 修订）。
    QMenu* settingsMenu = menuBar()->addMenu(tr("&Settings"));
    QMenu* logLevelMenu = settingsMenu->addMenu(tr("Log Level(&L)"));
    logLevelMenu->setToolTipsVisible(true);

    const char* const kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    // 批量开关（置于级别列表顶部，解决单级别勾选状态不一致/找不到入口）
    logLevelMenu->addSeparator();
    allLevelsAction_ = logLevelMenu->addAction(tr("Enable All"));
    noneLevelsAction_ = logLevelMenu->addAction(tr("Disable All"));
    connect(allLevelsAction_, &QAction::triggered, this, [this] { onAllLevels(true); });
    connect(noneLevelsAction_, &QAction::triggered, this, [this] { onAllLevels(false); });

    for (int i = 0; i < 5; ++i) {
        QAction* c = logLevelMenu->addAction(QString::fromLatin1(kLevelNames[i]));
        c->setCheckable(true);
        c->setChecked(i != 0);  // 默认矩阵：DEBUG 关、其余开
        c->setData(i);  // LogLevel 索引
        connect(c, &QAction::toggled, this, &MainWindow::onLevelToggled);
        levelActions_.append(c);
    }

    // VTK 日志拦截开关（FR-011；VTK 未引入，仅配置项）
    vtkLogAction_ = settingsMenu->addAction(tr("VTK Log Interception(&V)"));
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
    logPathAction_ = settingsMenu->addAction(tr("Log file: not configured"));
    logPathAction_->setEnabled(false);  // 只读展示（可选中复制），路径由 main.cpp 注入后更新
    openLogDirAction_ = settingsMenu->addAction(tr("Open Log Directory(&O)"));
    openLogDirAction_->setEnabled(false);  // 路径注入前不可用
    setActionIcon(openLogDirAction_, "tools-settings");  // 003：契约图标
    connect(openLogDirAction_, &QAction::triggered, this, &MainWindow::openLogDir);

    // 日志路径可配置（FR-016）：选择目录后迁移旧日志并持久化
    setLogPathAction_ = settingsMenu->addAction(tr("Set Log Path...(&P)"));
    setLogPathAction_->setEnabled(false);  // 路径注入前不可用
    setActionIcon(setLogPathAction_, "tools-settings");  // 003：契约图标
    connect(setLogPathAction_, &QAction::triggered, this, &MainWindow::setLogPath);
    // 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档
    clearLogAction_ = settingsMenu->addAction(tr("Clear Log History(&C)"));
    clearLogAction_->setEnabled(false);
    setActionIcon(clearLogAction_, "edit-delete-selection");  // 003：契约图标
    connect(clearLogAction_, &QAction::triggered, this, &MainWindow::clearLogHistory);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
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
    winCloseBtn_->setToolTip(tr("Close"));

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
        // 005-multi-screen-maximize：几何恢复统一由 changeEvent(WindowStateChange)
        // 完成（按钮/双击/任务栏右键/快捷键同一路径，FR-003/006）
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
    winMaxBtn_->setToolTip(max ? tr("Restore") : tr("Maximize"));
}

// ---- 功能栏（003：左侧通用 + 右侧领域，纵向 ToolButtonIconOnly）----
void MainWindow::createToolbars() {
    // 左：通用功能栏（FR-003：10 按钮 = 5 可用复用菜单动作 + 5 禁用占位，顺序契约 §2）
    leftToolBar_ = new QToolBar(tr("Left Toolbar"), this);
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
            act->setToolTip(spec->tooltip);  // FR-006：悬停提示
            leftToolBar_->addAction(act);
        }
    }
    // 右：领域功能栏（FR-005：9 按钮，全部禁用态占位，顺序契约 §3）
    rightToolBar_ = new QToolBar(tr("Right Toolbar"), this);
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
        act->setToolTip(spec->tooltip);  // FR-006：悬停提示
        setActionIcon(act, spec->iconId);
        rightToolBar_->addAction(act);
    }
}

// ---- 导出主界面图片（grab + PNG）----
void MainWindow::exportMainWindowImage() {
    // 默认目录跟随程序当前工作目录（= 启动程序时所在路径，FR-015）
    const QString defaultPath = QDir::current().filePath(QStringLiteral("perception.png"));
    const QString path = runThemedFileDialog(this, tr("Export Main Window Image"),
        defaultPath, tr("PNG Image (*.png)"), FileDialogMode::Save);
    if (path.isEmpty()) return;

    const QPixmap pm = grab();  // 抓取整个主窗口当前渲染（含菜单/Dock/状态栏）
    if (!pm.save(path, "PNG")) {
        QMessageBox::warning(this, tr("Export Failed"),
                             tr("Unable to write:\n%1").arg(path));
        return;
    }
    statusBar()->showMessage(tr("Exported main window image: %1").arg(QFileInfo(path).fileName()), 5000);
}

// ---- 导出控制台命令为 .py 脚本 ----
void MainWindow::exportPythonCommands() {
    const QStringList cmds = pythonConsole_->history();
    if (cmds.isEmpty()) {
        statusBar()->showMessage(tr("No commands to export"), 3000);
        return;
    }
    const QString path = runThemedFileDialog(this, tr("Export Python Commands"),
        QDir::current().filePath(QStringLiteral("console_commands.py")),
        tr("Python Script (*.py)"), FileDialogMode::Save);
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"),
                             tr("Unable to write:\n%1").arg(path));
        return;
    }
    QTextStream out(&f);
    out << "# Generated by Perception Python Console (" << cmds.size() << " commands)\n";
    for (const QString& c : cmds) {
        out << c << "\n";
    }
    f.close();
    statusBar()->showMessage(
        tr("Exported %1 command(s): %2").arg(cmds.size()).arg(QFileInfo(path).fileName()), 5000);
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

// 启动恢复（main.cpp 在 configure + addSink 后调用，FR-013）：
// 读全局矩阵 log/level/<LEVEL>；未设置时回退旧版 log/console/<LEVEL>（迁移保设置），再回退默认（DEBUG 关、其余开）。
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
        logPathAction_->setText(tr("Log file: %1").arg(path));
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
        QMessageBox::warning(this, tr("Cannot Open Log Directory"),
                             tr("Cannot open directory: %1").arg(dir));
    }
}

// ---- 设置日志路径（FR-016）：选目录 -> 迁移旧日志 -> 重建 FileSink -> 持久化 ----
void MainWindow::setLogPath() {
    if (logFilePath_.isEmpty()) return;
    const QString curDir = QFileInfo(logFilePath_).absolutePath();
    const QString dir = runThemedFileDialog(this, tr("Select Log Directory"),
        curDir, QString(), FileDialogMode::Directory);
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
        ok ? tr("Log path changed and history migrated: %1").arg(newPath)
           : tr("Log path set (failed to migrate old logs): %1").arg(newPath), 6000);
}

// ---- 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档 ----
void MainWindow::clearLogHistory() {
    if (logFilePath_.isEmpty()) return;
    const QString dir = QFileInfo(logFilePath_).absolutePath();
    const auto ret = QMessageBox::question(
        this, tr("Clear Log History"),
        tr("This will delete all log files (including archives) in the log directory.\n\nDirectory: %1\n\nContinue?").arg(dir),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const bool ok = perception::core::log::Logger::instance().clearLogFiles();
    if (ok) {
        PERCEPTION_LOG_I("log history cleared");
        statusBar()->showMessage(tr("Cleared log history: %1").arg(dir), 5000);
    } else {
        QMessageBox::warning(this, tr("Clear Failed"),
                             tr("Unable to clear log files. Check directory permissions or disk status."));
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
    statusBar()->showMessage(tr("Layout reset"), 3000);
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
        tr("Theme switched to: %1").arg(QString::fromUtf8(t->name)), 3000);
}

// ---- Python 运行时释放（main 退出前调用）----
void MainWindow::shutdownPython() {
    if (pythonConsole_) pythonConsole_->shutdown();
}

// ---- Dock 面板 ----
void MainWindow::createDocks() {
    // 左：文件/数据集树（ParaView Pipeline 式，ui-guidelines §5）
    fileDock_ = new QDockWidget(tr("Data"), this);
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
    propertyDock_ = new QDockWidget(tr("Properties"), this);
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
    pythonDock_ = new QDockWidget(tr("Python Console"), this);
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
    exportBtn->setText(tr("Export"));
    exportBtn->setToolTip(tr("Export executed commands to a .py script"));
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setFixedSize(64, 28);
    connect(exportBtn, &QToolButton::clicked,
            this, &MainWindow::exportPythonCommands);

    auto* clearBtn = new QToolButton(sideBar);
    clearBtn->setObjectName(QStringLiteral("pythonClearBtn"));
    clearBtn->setText(tr("Clear"));
    clearBtn->setToolTip(tr("Clear console output (keeps defined variables)"));
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

    // 抑制 dock 键盘焦点 focus rect（说明见 NoFocusRectDockStyle）
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

// ---- 中央区域（004-dock-layout-manager：子窗口容器；空状态沿用占位语义）----
void MainWindow::createCentralArea() {
    subwindowContainer_ = new SubwindowContainer(this);
    subwindowContainer_->setEmptyHint(
        tr("Drop VTK / SVisual / HDF5 data files here\nor press Ctrl+O to open"));
    setCentralWidget(subwindowContainer_);
}

// ---- 004-dock-layout-manager：创建子窗口（FR-001/002，Python 桥 + 菜单双入口）----
void MainWindow::createSubwindow(const QString& title) {
    Q_UNUSED(title);  // 标题栏统一为 "plot_" + 递增序号（用户需求：plot_1、plot_2、…）
    ++subwindowSeq_;
    const QString displayTitle = QStringLiteral("plot_%1").arg(subwindowSeq_);
    auto* view = new SubwindowView(displayTitle, subwindowContainer_);
    subwindowContainer_->addSubwindow(view);

    // 子窗口交互接线（US6）：单击选中 / 最大化 / 隐藏 / 还原 / 向前向后切换
    connect(view, &SubwindowView::selected, this, [this, view] {
        // 选中即成为作用对象；仅对状态实际变化的 view 重新 apply QSS（unpolish/polish），
        // 避免反复刷新所有子窗口。状态未变的 view 直接跳过，性能更稳。
        for (auto* v : subwindowContainer_->subwindows()) {
            const bool sel = (v == view);
            if (v->property("selected").toBool() == sel) continue;
            v->setProperty("selected", sel);
            v->style()->unpolish(v);
            v->style()->polish(v);
            v->update();
        }
    });
    connect(view, &SubwindowView::maximizeRequested, this, [this, view] {
        if (subwindowContainer_->isMaximized()) {
            subwindowContainer_->exitMaximized();  // 已最大化：再次触发 = 退出（FR-016）
        } else {
            subwindowContainer_->setMaximized(view);  // 选中子窗口占满容器（中央区域）
        }
    });
    connect(view, &SubwindowView::hideRequested, this,
            [this, view] { subwindowContainer_->hideSubwindow(view); });
    connect(view, &SubwindowView::restoreRequested, this,
            [this] { subwindowContainer_->exitMaximized(); });
    connect(view, &SubwindowView::prevRequested, this,
            [this] { subwindowContainer_->cycleMaximized(-1); });
    connect(view, &SubwindowView::nextRequested, this,
            [this] { subwindowContainer_->cycleMaximized(1); });
    connect(view, &SubwindowView::closeRequested, this,
            [this, view] { subwindowContainer_->removeSubwindow(view); });  // 关闭 = 销毁（FR-002）

    statusBar()->showMessage(tr("Subwindow created: %1").arg(view->title()), 3000);
}

// ---- 004-dock-layout-manager：布局设置入口（US5；修改即生效 FR-011）----
void MainWindow::openLayoutSettings() {
    if (!layoutSettingsDialog_) {
        layoutSettingsDialog_ =
            new LayoutSettingsDialog(subwindowContainer_->layoutConfig(), this);
        layoutSettingsDialog_->setAttribute(Qt::WA_DeleteOnClose, false);
        connect(layoutSettingsDialog_, &LayoutSettingsDialog::configChanged, this,
                [this](const LayoutConfig& cfg) {
                    subwindowContainer_->setLayoutConfig(cfg);  // 即时重排（FR-011）
                });
    } else {
        // 打开时回显当前配置（US5）
        layoutSettingsDialog_->setConfig(subwindowContainer_->layoutConfig());
    }
    layoutSettingsDialog_->show();
    layoutSettingsDialog_->raise();
    layoutSettingsDialog_->activateWindow();
}

// ---- 004-dock-layout-manager：全屏协调（FR-017）----
// 全屏 = 中间区域（子窗口容器）扩展至整个主界面：隐藏三个 Dock 后 centralWidget
// 自动占满；退出时按全屏前记录恢复各 Dock 显隐。与子窗口最大化正交：
// 若已有子窗口最大化（checked），全屏时该子窗口随之占满整个主界面。
void MainWindow::setContainerFullscreen(bool on) {
    if (containerFullscreen_ == on) return;
    containerFullscreen_ = on;
    if (on) {
        docksVisibleBeforeFullscreen_.clear();
        for (QDockWidget* dock : {fileDock_, propertyDock_, pythonDock_}) {
            if (!dock) continue;
            docksVisibleBeforeFullscreen_.append(dock->toggleViewAction()->isChecked());
            dock->hide();
        }
    } else {
        int i = 0;
        for (QDockWidget* dock : {fileDock_, propertyDock_, pythonDock_}) {
            if (!dock) continue;
            if (i < docksVisibleBeforeFullscreen_.size())
                dock->setVisible(docksVisibleBeforeFullscreen_.at(i));
            ++i;
        }
        docksVisibleBeforeFullscreen_.clear();
    }
    if (toggleFullscreenAction_) toggleFullscreenAction_->setChecked(containerFullscreen_);
}

// ---- 004：恢复被"隐藏"的子窗口（View 菜单）----
void MainWindow::showHiddenSubwindows() {
    if (!subwindowContainer_) return;
    subwindowContainer_->showHiddenSubwindows();
    statusBar()->showMessage(tr("Hidden subwindows restored"), 3000);
}

// ---- 全屏/最大化：Esc 退出（FR-017）----
void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (containerFullscreen_) {
            setContainerFullscreen(false);  // 先退出全屏
            return;
        }
        if (subwindowContainer_ && subwindowContainer_->isMaximized()) {
            subwindowContainer_->exitMaximized();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

// ---- 状态栏 ----
void MainWindow::createStatusBar() {
    QStatusBar* sb = statusBar();
    sb->showMessage(tr("Ready"));

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
    const QString file = runThemedFileDialog(this, tr("Open Data File"),
        QDir::currentPath(),
        tr("VTK Files (*.vtk *.vti *.vtp *.vtu *.vts *.vtr);;"
           "SVisual Files (*.plt *.tdr);;"
           "HDF5 Files (*.h5 *.hdf5);;"
           "Curve Data (*.csv *.dat);;"
           "All Files (*)"),
        FileDialogMode::Open);
    if (file.isEmpty()) return;

    addFileToTree(file);
    statusBar()->showMessage(tr("Loaded: %1").arg(QFileInfo(file).fileName()), 5000);
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
        statusBar()->showMessage(tr("Loaded %1 file(s)").arg(added), 5000);
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
    showFramelessDialog(this, tr("About Perception"),
        tr("<h3>Perception %1</h3>"
           "<p>Desktop data visualization tool (inspired by ParaView / SVisual).</p>"
           "<p>Stack: C++17 / Qt / VTK / pybind11</p>")
            .arg(QApplication::applicationVersion()));
}

// ---- 无边框窗口状态同步 ----
void MainWindow::changeEvent(QEvent* e) {
    if (e->type() == QEvent::WindowStateChange) {
        // 005-multi-screen-maximize：最大化/还原几何状态管理（FR-003/006）。
        // normalGeometry_ 已在 WM_GETMINMAXINFO 记录（消息于窗口尺寸改变前到达，
        // frameGeometry 为最大化前几何，准确）；此处只做过渡判定与系统侧触发的还原：
        //   - 进入最大化：确保已记录（兜底取当前几何，覆盖极少数未走该消息的路径）
        //   - 退出最大化：还原最大化前几何（按钮/双击/任务栏右键/快捷键统一路径，
        //     原屏原尺寸，FR-003；若原屏已被断开，由系统将窗口归位到可见区域）
        const bool maximized = isMaximized();
        if (maximized && !prevMaximized_) {
            if (!normalGeometryValid_) {
                normalGeometry_ = frameGeometry();
                normalGeometryValid_ = !normalGeometry_.isEmpty();
            }
        } else if (!maximized && prevMaximized_ && normalGeometryValid_ &&
                   !normalGeometry_.isEmpty()) {
            setGeometry(normalGeometry_);
            normalGeometryValid_ = false;  // 一次性还原，再次最大化时由 MINMAXINFO 重新记录
        }
        prevMaximized_ = maximized;
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
        // 005-multi-screen-maximize：无边框最大化限制到窗口"主体所在"屏幕的工作区
        // （FR-001/002/004：任意屏幕、主/副屏一致、跨屏按窗口中心归属；不遮任务栏）。
        // 目标屏每次消息动态解析（不缓存屏幕指针），显示器热插拔/DPI 变化后仍正确
        // （FR-005，research R4）。
        auto* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        // 消息于窗口尺寸改变前到达：frameGeometry 仍为最大化前几何，
        // 顺带记录供 changeEvent 还原（FR-003/006）
        normalGeometry_ = frameGeometry();
        normalGeometryValid_ = !normalGeometry_.isEmpty();

        QList<QRect> screensGeom;
        QList<QRect> screensAvail;
        const QList<QScreen*> screens = QGuiApplication::screens();
        screensGeom.reserve(screens.size());
        screensAvail.reserve(screens.size());
        for (const QScreen* scr : screens) {
            screensGeom.append(scr->geometry());
            screensAvail.append(scr->availableGeometry());
        }
        const int idx =
            window_geometry::resolveTargetScreenIndex(screensGeom, normalGeometry_);
        const QRect target = window_geometry::maximizeGeometry(screensAvail, idx);
        if (!target.isEmpty()) {
            mmi->ptMaxPosition.x = target.x();
            mmi->ptMaxPosition.y = target.y();
            mmi->ptMaxSize.x = target.width();
            mmi->ptMaxSize.y = target.height();
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

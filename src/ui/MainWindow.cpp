// ===== Perception 主窗口实现（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// 规范：docs/design/ui-guidelines.md §5 布局范式 / §6 交互规范。
#include "ui/MainWindow.h"
#include "ui/frameless_dialog.h"

#include "core/io/file_type_catalog.h"

#include "ui/log/log_settings_controller.h"
#include "ui/maximize/window_maximize_controller.h"
#include "ui/subwindow/dock_drag_overlay.h"
#include "ui/subwindow/dock_title_bar.h"
#include "ui/themed_file_dialog.h"
#include "ui/themed_message_box.h"
#include "ui/win_btn_icon.h"

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
#include <functional>
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
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOption>
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



#include "ui/console/PythonConsole.h"
#include "ui/panellayout/panel_settings_dialog.h"
#include "ui/subwindow/layout_settings_dialog.h"
#include "ui/subwindow/subwindow_container.h"
#include "ui/subwindow/subwindow_view.h"
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

// 010-panel-layout-settings：DockArea → Qt::DockWidgetArea 映射（FR-003 应用层）
Qt::DockWidgetArea toQtDockArea(DockArea area) {
    switch (area) {
    case DockArea::Left:
        return Qt::LeftDockWidgetArea;
    case DockArea::Right:
        return Qt::RightDockWidgetArea;
    case DockArea::Bottom:
        return Qt::BottomDockWidgetArea;
    }
    return Qt::LeftDockWidgetArea;
}

// ---- 嵌入式 PyShell 顶部可拖拽边界（*Only 非全尺寸模式）----
// 作为 PyShell 区域的顶部边框线：1px 分隔线 + 一排 6 个点 handle，
// 全部位于 PyShell 与 Plot 的分界线上；hover/拖拽时用 WindowText 高亮
// （与 QMainWindow dock separator / DockDragOverlay sash 高亮一致）。
class EmbeddedConsoleSash : public QWidget {
public:
    explicit EmbeddedConsoleSash(QWidget* host, QWidget* parent)
        : QWidget(parent), host_(host) {
        setObjectName(QStringLiteral("consoleSash"));
        setCursor(Qt::SizeVerCursor);
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
        // 1px 视觉边界线 + 上下各 3px 命中余量，保证鼠标容易抓
        setFixedHeight(6);
    }

    // 拖拽后的目标高度回调（MainWindow 用于记录用户偏好）
    void setHeightListener(const std::function<void(int)>& fn) { onHeightChanged_ = fn; }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        const QPalette pal = palette();
        const bool active = dragging_ || hover_;
        // 颜色：常态/高亮都用 WindowText（与 DockDragOverlay 的 sash 高亮一致），
        // 浅色主题=深色、深色主题=白色，任何背景下都清晰可见。
        const QColor color = pal.color(QPalette::WindowText);
        p.setPen(Qt::NoPen);
        p.setBrush(color);

        // 底部分隔线 = PyShell 区域的顶部边框线（与 Plot 的分界）；hover/拖拽时加粗
        const int lineH = active ? 2 : 1;
        p.fillRect(QRect(0, height() - lineH, width(), lineH), color);

        // 6 个点 handle：一排 6 个 2x2 像素方块，居中于边界线上方
        constexpr int kDot = 2;
        constexpr int kGap = 3;
        constexpr int kCount = 6;
        const int totalW = (kCount - 1) * kGap + kDot;
        const int cx = (width() - totalW) / 2;
        const int y = height() - 1 - kDot;  // 点底部贴边界线，整体在线上方
        for (int i = 0; i < kCount; ++i)
            p.fillRect(cx + i * kGap, y, kDot, kDot, color);
    }

    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() != Qt::LeftButton) return;
        dragging_ = true;
        pressY_ = e->globalPos().y();
        startHeight_ = host_->height();
        grabMouse();
        update();
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!dragging_) return;
        // 向上拖动（y 减小）→ PyShell 应变高；向下拖动（y 增大）→ PyShell 应变矮。
        const int delta = e->globalPos().y() - pressY_;
        const int minH = 120;  // 标题栏 + 控制台最小高度
        const int maxH = host_->parentWidget()
                             ? qMax(minH, host_->parentWidget()->height() - 40)
                             : minH;
        const int h = qBound(minH, startHeight_ - delta, maxH);
        host_->setFixedHeight(h);
        if (onHeightChanged_) onHeightChanged_(h);
    }

    void mouseReleaseEvent(QMouseEvent*) override {
        if (!dragging_) return;
        dragging_ = false;
        releaseMouse();
        update();
    }

    bool event(QEvent* e) override {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave) {
            hover_ = (e->type() == QEvent::HoverEnter);
            update();
        }
        return QWidget::event(e);
    }

private:
    QWidget* host_;          // 被调整高度的嵌入式 PyShell 槽位
    bool dragging_ = false;
    bool hover_ = false;
    int pressY_ = 0;
    int startHeight_ = 0;
    std::function<void(int)> onHeightChanged_;
};

// 日志级别持久化 key（FR-013）：全局单一矩阵（FR-012 修订，不再区分控制台/文件）。
// 兼容：旧版本曾分别持久化 log/console/<LEVEL> 与 log/file/<LEVEL>；升级后优先读新
// 全局 key，未设置时回退到旧 log/console/<LEVEL>，保证既有用户设置不丢。
constexpr const char* kLogLevelPrefix         = "log/level/";
constexpr const char* kLogLegacyConsolePrefix = "log/console/";
constexpr const char* kLogVtkEnabled          = "log/vtkEnabled";
constexpr const char* kLogPathKey       = "log/path";  // 用户配置的日志文件路径（FR-016）
// 010-panel-layout-settings：面板布局持久化 key 引用 panel_layout_config.h 的 inline
// 常量（FR-007；单一事实源，勿在此重复定义）

// 动作图标统一构造（makeActionIcon）随装配函数移至 MainWindow_assembly.cpp。

// 窗口控制按钮图标见 ui/win_btn_icon.h（统一 16px 矢量绘制；
// 文本字符字形/基线不一观感不齐，故全部用 QPainter 图标）。
// 通用对话框标题栏工厂 buildDialogTitleBar 见 ui/dialog_title_bar.h
//（FramelessDialog / ThemedFileDialog / LayoutSettingsDialog 共享）。

// ---- 文件/目录对话框 ----
// 006-constitution-refactor：ThemedFileDialog / runThemedFileDialog 已提取为共享模块
//（ui/themed_file_dialog.h/.cpp），MainWindow 与 LogSettingsController 共用。

// ---- 帮助/关于弹窗 ----
// 006-constitution-refactor：FramelessDialog / showFramelessDialog 已提取为
// ui/frameless_dialog.h/.cpp（MainWindow 与装配函数共用）。

// ---- 自定义 Dock 标题栏 ----
// 006-constitution-refactor：DockTitleBar / NoFocusRectDockStyle / applyNoFocusRectStyle /
// wrapWithSizeGrip 已提取至 ui/subwindow/dock_title_bar.h/.cpp（Dock 装配职责下放）。

// ---- 分界线高亮细条 ----
// 006-constitution-refactor：SashHighlight / DragOverlayWidget 已随控制器提取至
// dock_drag_overlay.cpp（匿名 namespace）。

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Perception"));
    // 无边框：舍弃系统标题栏，标题/窗口按钮与菜单栏同一行（createTitleBar 组装）
    // 最大化时通过 WM_GETMINMAXINFO 限制到工作区，避免覆盖任务栏
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1360, 860);
    // 006-constitution-refactor：日志设置控制器须在 createMenus 之前创建
    //（createMenus → attachLogMenu 使用它；原放置构造函数末尾导致空指针崩溃）
    logSettingsController_ = new LogSettingsController(this, this);

    createActions();
    createMenus();
    createTitleBar();  // 需在 createMenus 之后（组装 menuBar()）
    createToolbars();
    createDocks();
    createCentralArea();
    createStatusBar();

    // 004-dock-layout-manager：Python create_window 桥 → 回调注册（FR-001，契约
    // contracts/python-create-window.md；REPL 在 GUI 线程执行，回调返回生成的窗口 id）
    pythonConsole_->setCreateWindowCallback(
        [this](const QString& title) { return createSubwindow(title); });

    // Dock 布局记忆（ui-guidelines §5.1：下次打开还原布局）
    QSettings settings;
    restoreGeometry(settings.value(kSettingsGeometryKey).toByteArray());
    restoreState(settings.value(kSettingsLayoutKey).toByteArray());

    // 010-panel-layout-settings：启动恢复面板布局持久化（FR-007/SC-005；
    // 与 applyPanelLayout 同语义：模式→区域 + 显隐 expand；key 值域见 data-model §5）
    {
        PanelLayoutConfig cfg;
        cfg.mode = modeFromKey(settings.value(QLatin1String(kPanelSettingsModeKey)).toString(),
                               PanelLayoutMode::DualWithConsole);
        cfg.dataVisible = settings.value(QLatin1String(kPanelSettingsDataKey), true).toBool();
        cfg.propertyVisible =
            settings.value(QLatin1String(kPanelSettingsPropertyKey), true).toBool();
        cfg.consoleVisible = settings.value(QLatin1String(kPanelSettingsConsoleKey), true).toBool();
        applyPanelLayout(cfg);
    }

    // 006-constitution-refactor：拆分出的控制器（多屏最大化 / 拖拽高亮）
    //（logSettingsController_ 已移至构造函数开头，见 createMenus 依赖说明）
    windowMaximizeController_ = new WindowMaximizeController(this, this);
    dockDragOverlay_ = new DockDragOverlay(this, this);
    connect(windowMaximizeController_, &WindowMaximizeController::maximizedChanged,
            this, [this](bool) { updateWindowButtonIcons(); });

    setAcceptDrops(true);  // 拖放打开（ui-guidelines §6）
    updateEmptyHints();

}

// ---- 分界线（dock 边缘分隔条）resize 拖拽高亮 ----
// 006-constitution-refactor：命中检测与高亮已提取至 DockDragOverlay 控制器
//（dock_drag_overlay.h/.cpp），MainWindow::event 转发驱动。
// 006-constitution-refactor：createActions / createMenus / createTitleBar /
// createToolbars / 动作图标注册（setActionIcon 等）已移至 MainWindow_assembly.cpp。

// ---- 菜单栏 ----
// ---- 无边框自定义标题栏 ----
// 背景：舍弃系统标题栏（Qt::FramelessWindowHint），将"应用图标+标题+拖拽区"、
//       菜单栏、最小化/最大化/关闭按钮组装为一行（VSCode 式紧凑布局）。
// 交互（Windows，nativeEvent 处理）：
//   - 标题栏拖拽区 -> HTCAPTION（系统拖动窗口 + 双击最大化）
//   - 窗口边缘 5px -> HTLEFT/HTTOP/...（边缘调整大小）
//   - WM_GETMINMAXINFO -> 最大化限制到工作区（不遮任务栏）
// ---- 导出主界面图片（grab + PNG）----
void MainWindow::exportMainWindowImage() {
    // 默认目录跟随程序当前工作目录（= 启动程序时所在路径，FR-015）
    const QString defaultPath = QDir::current().filePath(QStringLiteral("perception.png"));
    const QString path = runThemedFileDialog(this, tr("Export Main Window Image"),
        defaultPath, tr("PNG Image (*.png)"), FileDialogMode::Save);
    if (path.isEmpty()) return;

    const QPixmap pm = grab();  // 抓取整个主窗口当前渲染（含菜单/Dock/状态栏）
    if (!pm.save(path, "PNG")) {
        showThemedMessageBox(this, QMessageBox::Warning, tr("Export Failed"),
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
        showThemedMessageBox(this, QMessageBox::Warning, tr("Export Failed"),
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

// ---- 日志设置（006-constitution-refactor：逻辑在 LogSettingsController）----
// 级别矩阵应用/持久化、路径展示/迁移、打开目录、清除历史已提取至控制器；
// 此处仅保留 main.cpp 契约入口（转发）。
void MainWindow::restoreLogSettings() {
    if (logSettingsController_) logSettingsController_->restoreFromSettings();
}

void MainWindow::setLogFilePath(const QString& path) {
    if (logSettingsController_) logSettingsController_->setLogFilePath(path);
}

// ---- 重置布局（默认布局恢复，供 Ctrl+Shift+L / --snapshot 模式调用）----
void MainWindow::resetLayout() {
    // 010-panel-layout-settings：默认 = DualWithConsole（PyShell 全尺寸 dock 宿主）
    // 清除嵌入式模式的临时全宽覆盖态（若有）
    consoleFullWidthOverride_ = false;
    if (pythonDockTitleBar_) pythonDockTitleBar_->setFullWidthConsole(false);
    setConsoleEmbedded(false);
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
    // 010-panel-layout-settings：重置面板布局持久化并回到默认配置（FR-007 联动；
    // 默认 = DualWithConsole + 三面板全显，与上述 addDockWidget/setVisible 一致）
    QSettings ps;
    ps.remove(kPanelSettingsModeKey);
    ps.remove(kPanelSettingsDataKey);
    ps.remove(kPanelSettingsPropertyKey);
    ps.remove(kPanelSettingsConsoleKey);
    lastPanelLayoutConfig_ = PanelLayoutConfig();
    statusBar()->showMessage(tr("Layout reset"), 3000);
}

// ---- 010-panel-layout-settings：PyShell 宿主切换 ----
// 全尺寸（*WithConsole）→ PyShell 挂回底部 dock（横跨全宽，两侧面板在其上方）；
// 非全尺寸（*Only）→ PyShell 嵌入中央区下方窄条（只占中央宽度，两侧面板保持全高）。
// pythonConsoleContainer_ 在 dock 的 wrap 包装与 consoleEmbeddedHost_ 槽位间 reparent，
// PythonConsole 及其按钮栏随容器移动，内容与状态不丢失。
void MainWindow::setConsoleEmbedded(bool embedded) {
    if (!pythonConsoleContainer_ || !pythonConsoleWrapper_ || !consoleEmbeddedHost_) return;
    consoleEmbeddedActive_ = embedded;

    // 1) 从 dock 槽位移出（若 PyShell 当前在 dock）
    if (pythonDock_->widget()) pythonDock_->setWidget(nullptr);

    // 2) 从嵌入式槽位移出 PyShell 内容容器（保留标题栏 widget）。
    // createCentralArea 在槽位顶部插入了静态标题栏（QWidget#dockTitleBar），
    // 切换宿主时只移除内容容器 pythonConsoleContainer_，不能误删标题栏。
    if (auto* lay = consoleEmbeddedHost_->layout()) {
        QLayoutItem* contentItem = nullptr;
        for (int i = 0; i < lay->count(); ++i) {
            if (lay->itemAt(i)->widget() == pythonConsoleContainer_) {
                contentItem = lay->takeAt(i);
                break;
            }
        }
        if (contentItem) {
            pythonConsoleContainer_->setParent(nullptr);
            delete contentItem;
        }
    }

    // 3) 按目标宿主安置（layout 接管 reparent；wrap 包装 dock 单例复用）
    if (embedded) {
        pythonConsoleContainer_->setParent(consoleEmbeddedHost_);
        // stretch=1：内容占满标题栏下方的剩余高度（约 180px）
        if (auto* box = qobject_cast<QBoxLayout*>(consoleEmbeddedHost_->layout())) {
            box->addWidget(pythonConsoleContainer_, 1);
        } else {
            consoleEmbeddedHost_->layout()->addWidget(pythonConsoleContainer_);
        }
        pythonConsoleWrapper_->hide();
    } else {
        pythonConsoleContainer_->setParent(pythonConsoleWrapper_);
        // wrap 包装为 QGridLayout（wrapWithSizeGrip）：container 横跨 grip 行列，
        // 与初始布局一致，右下角 size grip 叠放不挤开内容
        if (auto* grid = qobject_cast<QGridLayout*>(pythonConsoleWrapper_->layout())) {
            grid->addWidget(pythonConsoleContainer_, 0, 0, 2, 2);
        } else {
            pythonConsoleWrapper_->layout()->addWidget(pythonConsoleContainer_);
        }
        pythonDock_->setWidget(pythonConsoleWrapper_);
    }
}

// ---- 010-panel-layout-settings：PyShell 全宽覆盖（嵌入式模式的"最大化/恢复"）----
// *Only（非全尺寸）模式下，标题栏"最大化"把 PyShell 临时展开为全尺寸底部 dock
//（不改变模式与左右面板），dock 标题栏此时显示"恢复"（DockTitleBar 覆盖态）可收回嵌入式。
void MainWindow::setConsoleFullWidth(bool fullWidth) {
    if (consoleFullWidthOverride_ == fullWidth) return;
    consoleFullWidthOverride_ = fullWidth;

    // 010-panel-layout-settings：setFloating(false) 会发射 topLevelChanged(false)；
    // 若此时自动恢复槽再回调 setConsoleFullWidth(false)，会造成递归自我撤销，
    // 导致 PyShell 内容被移出 dock 而外框仍显示。执行期间临时阻塞 pythonDock_ 信号。
    const QSignalBlocker blocker(pythonDock_);

    if (fullWidth) {
        // 嵌入式 → 全尺寸底部 dock（内容随 setConsoleEmbedded 迁移到 wrap 包装）
        consoleEmbeddedHost_->setVisible(false);
        setConsoleEmbedded(false);
        // 非全尺寸（*Only）模式时 pythonDock_ 已被 applyPanelLayout remove 出主窗口；
        // 必须重新挂回底部 dock area，否则"分离→Re-dock（setFloating(false)）"无处停靠，
        // 且浮动的 PyShell 也无法复原（无 dock 槽位可归位）。
        addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);
        pythonDock_->setVisible(true);
        pythonDock_->setFloating(false);
        pythonDock_->raise();
    } else {
        // 全尺寸 dock → 嵌入式窄条
        pythonDock_->setVisible(false);
        setConsoleEmbedded(true);
        consoleEmbeddedHost_->setVisible(true);
    }
    if (pythonDockTitleBar_) pythonDockTitleBar_->setFullWidthConsole(consoleFullWidthOverride_);
    if (togglePythonConsoleAction_) togglePythonConsoleAction_->setChecked(true);
}

// ---- 010-panel-layout-settings：面板布局应用（FR-003/FR-005）----
// 模式 → 区域：removeDockWidget 后按 targetArea 重新 addDockWidget（Data/Property 左右
// 互换）；PyShell 按尺寸形态落位——全尺寸（*WithConsole）= 底部 dock 全宽，
// 非全尺寸（*Only）= 嵌入中央下方窄条。显隐 → setVisible（expand：隐藏面板的空间由
// 可见面板与中央区按比例吸收，无空白死区）；resizeDocks 仅对可见面板调用。
// 同步更新 toggle 动作勾选态。
void MainWindow::applyPanelLayout(const PanelLayoutConfig& cfg) {
    lastPanelLayoutConfig_ = cfg;
    // 全屏态兼容：全屏时三 dock 已隐藏，先退出全屏再重排（布局面板设置在全屏下无意义）
    if (containerFullscreen_) setContainerFullscreen(false);
    // 模式重排是权威配置：清除嵌入式模式的临时"全宽覆盖"态（PyShell 标题栏最大化/恢复）
    if (consoleFullWidthOverride_) {
        consoleFullWidthOverride_ = false;
        if (pythonDockTitleBar_) pythonDockTitleBar_->setFullWidthConsole(false);
    }

    const bool showData = isPanelVisible(cfg, PanelId::Data);
    const bool showProperty = isPanelVisible(cfg, PanelId::Property);
    const bool showConsole = isPanelVisible(cfg, PanelId::PyShell);
    // PyShell 尺寸形态：*WithConsole = 全尺寸（底部全宽 dock）；*Only = 非全尺寸（嵌入式窄条）
    const bool fullWidth = modeHasFullWidthConsole(cfg.mode);

    // PyShell 宿主切换（先于 dock 重排：嵌入式时 PyShell 不占用 bottom dock 空间）
    setConsoleEmbedded(showConsole && !fullWidth);

    removeDockWidget(fileDock_);
    removeDockWidget(propertyDock_);
    removeDockWidget(pythonDock_);

    addDockWidget(toQtDockArea(targetArea(cfg.mode, PanelId::Data)), fileDock_);
    addDockWidget(toQtDockArea(targetArea(cfg.mode, PanelId::Property)), propertyDock_);
    if (showConsole && fullWidth) addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);

    // 显隐（FR-005：隐藏面板空间被其余可见面板与中央区吸收）
    fileDock_->setVisible(showData);
    propertyDock_->setVisible(showProperty);
    pythonDock_->setVisible(showConsole && fullWidth);
    if (consoleEmbeddedHost_) consoleEmbeddedHost_->setVisible(showConsole && !fullWidth);

    // 非浮动 + 尺寸规整（仿 resetLayout；仅可见面板，隐藏时 QMainWindow 自动重分配）
    for (QDockWidget* dock : {fileDock_, propertyDock_, pythonDock_}) {
        if (dock->isVisible()) dock->setFloating(false);
    }
    if (fileDock_->isVisible()) resizeDocks({fileDock_}, {240}, Qt::Horizontal);
    if (propertyDock_->isVisible()) resizeDocks({propertyDock_}, {280}, Qt::Horizontal);
    if (pythonDock_->isVisible()) resizeDocks({pythonDock_}, {200}, Qt::Vertical);

    // 同步 View 菜单显隐动作勾选态（与配置一致，而非 dock 实际可见性：嵌入式 PyShell
    // 不占用 dock，Python Console 显隐状态仍由该动作表达）
    toggleFileDockAction_->setChecked(showData);
    togglePropertyDockAction_->setChecked(showProperty);
    togglePythonConsoleAction_->setChecked(showConsole);
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
    // 嵌入式 PyShell 标题栏窗口按钮图标随主题刷新（DockTitleBar 自身处理 palette change）
    const QPalette pal = palette();
    if (embedMaxBtn_) embedMaxBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Maximize, pal));
    if (embedFloatBtn_) embedFloatBtn_->setIcon(makeWinBtnIcon(WinBtnKind::FloatDock, pal));
    if (embedCloseBtn_) embedCloseBtn_->setIcon(makeWinBtnIcon(WinBtnKind::Close, pal));

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
    // 010-panel-layout-settings：容器提升为成员，可在 dock（全尺寸 *WithConsole）与
    // 中央下方嵌入式槽位（非全尺寸 *Only）之间 reparent 切换宿主
    pythonConsoleContainer_ = new QWidget(pythonDock_);
    pythonConsoleContainer_->setObjectName(QStringLiteral("pythonConsoleContainer"));
    auto* pythonLayout = new QHBoxLayout(pythonConsoleContainer_);
    pythonLayout->setContentsMargins(0, 0, 0, 0);
    pythonLayout->setSpacing(0);

    pythonConsole_ = new PythonConsole(pythonConsoleContainer_);
    pythonLayout->addWidget(pythonConsole_, 1);

    auto* sideBar = new QWidget(pythonConsoleContainer_);
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

    sideLayout->addStretch();          // 顶部弹性间隔：Export 不再紧贴 top margin
    sideLayout->addWidget(exportBtn);
    sideLayout->addStretch();          // Export 与 Clear 之间弹性间隔
    sideLayout->addWidget(clearBtn);   // Clear 贴底（保留 bottom margin 8）
    sideLayout->addStretch(); 
    pythonLayout->addWidget(sideBar);

    // 010-panel-layout-settings：wrap 包装保存为成员（dock 宿主单例，嵌入模式时复用）
    pythonConsoleWrapper_ = wrapWithSizeGrip(pythonConsoleContainer_, pythonDock_);
    pythonDock_->setWidget(pythonConsoleWrapper_);
    // 010-panel-layout-settings：保存标题栏指针——嵌入式模式的"全宽覆盖"态需切换其
    // 最大化按钮为"恢复嵌入式"（setConsoleFullWidth 驱动）
    pythonDockTitleBar_ = new DockTitleBar(pythonDock_);
    pythonDock_->setTitleBarWidget(pythonDockTitleBar_);
    addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);
    // 010：*Only（非全尺寸）模式 PyShell 分离（临时全宽覆盖态）后 Re-dock
    //（浮动态→停靠态）——直接收回嵌入式窄条，对应当前选中的模式，而不是停在
    // "底部全宽 dock"的初始布局形态（默认 DualWithConsole 视觉）。全宽模式
    //（consoleFullWidthOverride_=false）或用户已点"恢复嵌入"后不受影响。
    connect(pythonDock_, &QDockWidget::topLevelChanged, this, [this](bool topLevel) {
        if (!topLevel && consoleFullWidthOverride_ &&
            !modeHasFullWidthConsole(lastPanelLayoutConfig_.mode)) {
            setConsoleFullWidth(false);
        }
    });

    // 抑制 dock 键盘焦点 focus rect（说明见 NoFocusRectDockStyle）
    applyNoFocusRectStyle(fileDock_);
    applyNoFocusRectStyle(propertyDock_);
    applyNoFocusRectStyle(pythonDock_);
    connect(togglePythonConsoleAction_, &QAction::triggered, this, [this](bool on) {
        // 010-panel-layout-settings：PyShell 双宿主——底部 dock（全尺寸 *WithConsole）
        // 或中央下方嵌入式槽位（非全尺寸 *Only），按当前生效宿主切换显隐。
        // 用 consoleEmbeddedActive_ 判断（槽位内常驻分隔条/标题栏，不能按 layout count）
        if (consoleEmbeddedActive_ && consoleEmbeddedHost_)
            consoleEmbeddedHost_->setVisible(on);
        else
            pythonDock_->setVisible(on);
    });
    connect(pythonDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        togglePythonConsoleAction_->setChecked(visible);
    });
    togglePythonConsoleAction_->setChecked(true);

    // 重置布局
    connect(resetLayoutAction_, &QAction::triggered, this, &MainWindow::resetLayout);
}

// ---- Dock 拖拽高亮 / 分隔条拖拽高亮（006-constitution-refactor：转发到控制器）----
// 高亮覆盖层、命中检测、放置逻辑已提取至 DockDragOverlay 控制器（dock_drag_overlay.cpp）；
// 此处保留 DockTitleBar 拖拽回调契约入口。
void MainWindow::beginDockDrag(QDockWidget* dock) {
    if (dockDragOverlay_) dockDragOverlay_->beginDockDrag(dock);
}

void MainWindow::updateDockDrag(const QPoint& globalPos) {
    if (dockDragOverlay_) dockDragOverlay_->updateDockDrag(globalPos);
}

void MainWindow::endDockDrag(const QPoint& globalPos) {
    if (dockDragOverlay_) dockDragOverlay_->endDockDrag(globalPos);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (dockDragOverlay_) dockDragOverlay_->syncToWindowSize();
}

// ---- 中央区域（004-dock-layout-manager：子窗口容器；空状态沿用占位语义）----
// 010-panel-layout-settings：中央区 = 子窗口容器 + 底部嵌入式 PyShell 槽位
//（非全尺寸 *Only 模式 PyShell 嵌入 Plot 下方窄条、两端抵在两侧面板，两侧面板保持全高）
void MainWindow::createCentralArea() {
    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralAreaContainer"));
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    subwindowContainer_ = new SubwindowContainer(central);
    subwindowContainer_->setEmptyHint(
        tr("Drop VTK / SVisual / HDF5 data files here\nor press Ctrl+O to open"));
    v->addWidget(subwindowContainer_, 1);

    // 嵌入式 PyShell 槽位（默认隐藏；*Only 模式由 applyPanelLayout 显示并装入容器）
    consoleEmbeddedHost_ = new QWidget(central);
    consoleEmbeddedHost_->setObjectName(QStringLiteral("consoleEmbeddedHost"));
    // QWidget 默认不绘制 QSS border/background；启用 styled background 让
    // theme_template.qss 中的 border 与 background-color 在 host 上实际生效。
    consoleEmbeddedHost_->setAttribute(Qt::WA_StyledBackground, true);
    consoleEmbeddedHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 非全尺寸窄条默认高度 = 分隔条（6px）+ 标题栏（约 26px）+ 内容约 180px；
    // 用户可通过顶部分隔条拖拽调整（EmbeddedConsoleSash）
    consoleEmbeddedHost_->setFixedHeight(212);
    auto* embedLayout = new QVBoxLayout(consoleEmbeddedHost_);
    embedLayout->setContentsMargins(0, 0, 0, 0);
    embedLayout->setSpacing(0);

    // 标题栏：与底部 dock 模式视觉一致（QSS 复用 QWidget#dockTitleBar / QLabel#dockTitleLabel）
    // + 与 DockTitleBar 同款功能按钮（复用 dockMaxBtn/dockFloatBtn/dockCloseBtn QSS）：
    //   最大化/恢复 = 嵌入式 ↔ 全尺寸底部 dock 切换；浮动 = 分离为独立窗口；关闭 = 隐藏。
    // 顶部内嵌可拖拽边界（EmbeddedConsoleSash），让 PyShell 与 Plot 的分界线就是标题栏顶边。
    auto* embedTitle = new QWidget(consoleEmbeddedHost_);
    embedTitle->setObjectName(QStringLiteral("dockTitleBar"));
    auto* embedTitleOuterLayout = new QVBoxLayout(embedTitle);
    embedTitleOuterLayout->setContentsMargins(0, 0, 0, 0);
    embedTitleOuterLayout->setSpacing(0);

    // 可拖拽边界（PyShell 与 Plot 的分界）：1px 线 + 6 个点，位于标题栏顶部
    auto* embedSash = new EmbeddedConsoleSash(consoleEmbeddedHost_, embedTitle);
    embedSash->setHeightListener([this](int h) {
        Q_UNUSED(h);
        // 拖拽后 PyShell 高度保持到下次显示（宿主自带高度，无需额外存储）
        if (dockDragOverlay_) dockDragOverlay_->syncToWindowSize();
    });
    embedTitleOuterLayout->addWidget(embedSash);

    // 标题栏按钮行
    auto* embedTitleBtnRow = new QWidget(embedTitle);
    auto* embedTitleBtnLayout = new QHBoxLayout(embedTitleBtnRow);
    embedTitleBtnLayout->setContentsMargins(10, 0, 4, 0);
    embedTitleBtnLayout->setSpacing(2);
    auto* embedTitleLabel = new QLabel(pythonDock_->windowTitle(), embedTitleBtnRow);
    embedTitleLabel->setObjectName(QStringLiteral("dockTitleLabel"));
    embedTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    embedTitleBtnLayout->addWidget(embedTitleLabel);

    auto makeEmbedBtn = [this, embedTitleBtnRow](const QString& objName, WinBtnKind kind) {
        auto* btn = new QToolButton(embedTitleBtnRow);
        btn->setObjectName(objName);
        btn->setFixedSize(20, 20);
        btn->setIconSize(QSize(16, 16));
        btn->setIcon(makeWinBtnIcon(kind, palette()));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        return btn;
    };
    embedMaxBtn_ = makeEmbedBtn(QStringLiteral("dockMaxBtn"), WinBtnKind::Maximize);
    embedMaxBtn_->setToolTip(tr("Maximize"));
    connect(embedMaxBtn_, &QToolButton::clicked, this, [this] {
        setConsoleFullWidth(!consoleFullWidthOverride_);
    });
    embedFloatBtn_ = makeEmbedBtn(QStringLiteral("dockFloatBtn"), WinBtnKind::FloatDock);
    embedFloatBtn_->setToolTip(tr("Undock"));
    connect(embedFloatBtn_, &QToolButton::clicked, this, [this] {
        setConsoleFullWidth(true);   // 先入全尺寸 dock（内容随 setConsoleEmbedded 迁移）
        if (pythonDock_) {
            pythonDock_->setFloating(true);
            pythonDock_->raise();
        }
    });
    embedCloseBtn_ = makeEmbedBtn(QStringLiteral("dockCloseBtn"), WinBtnKind::Close);
    embedCloseBtn_->setToolTip(tr("Close"));
    connect(embedCloseBtn_, &QToolButton::clicked, this, [this] {
        consoleEmbeddedHost_->setVisible(false);
        if (togglePythonConsoleAction_) togglePythonConsoleAction_->setChecked(false);
    });
    embedTitleBtnLayout->addWidget(embedMaxBtn_);
    embedTitleBtnLayout->addWidget(embedFloatBtn_);
    embedTitleBtnLayout->addWidget(embedCloseBtn_);
    embedTitleOuterLayout->addWidget(embedTitleBtnRow);
    embedLayout->addWidget(embedTitle);

    consoleEmbeddedHost_->hide();
    v->addWidget(consoleEmbeddedHost_);

    setCentralWidget(central);
}

// ---- 004-dock-layout-manager：创建子窗口（FR-001/002，Python 桥 + 菜单双入口）----
// 返回生成的窗口 id（"Plot_" + 全局递增序号，用户需求 2026-08-29）；
// title 缺省（空白）时窗口标题 = id，传入 title 时窗口标题显示 title。
QString MainWindow::createSubwindow(const QString& title) {
    ++subwindowSeq_;
    const QString id = QStringLiteral("Plot_%1").arg(subwindowSeq_);
    const QString displayTitle = title.trimmed().isEmpty() ? id : title.trimmed();
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
    return id;
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
        // 008 WS2：打开期间子窗口增删时同步实时预览计数（FR-013）
        connect(subwindowContainer_, &SubwindowContainer::subwindowCountChanged, this,
                [this](int) {
                    if (layoutSettingsDialog_->isVisible()) {
                        layoutSettingsDialog_->setPreviewCount(
                            subwindowContainer_->visibleSubwindowCount());
                    }
                });
    } else {
        // 打开时回显当前配置（US5）
        layoutSettingsDialog_->setConfig(subwindowContainer_->layoutConfig());
    }
    // 实时预览计数 = 当前可见子窗口数（FR-013）
    layoutSettingsDialog_->setPreviewCount(subwindowContainer_->visibleSubwindowCount());
    layoutSettingsDialog_->show();
    layoutSettingsDialog_->raise();
    layoutSettingsDialog_->activateWindow();
}

// ---- 010-panel-layout-settings：面板布局设置入口（FR-001；US3 实时预览）----
void MainWindow::openPanelSettings() {
    if (!panelSettingsDialog_) {
        panelSettingsDialog_ =
            new PanelSettingsDialog(currentPanelLayoutConfig(), this);
        panelSettingsDialog_->setAttribute(Qt::WA_DeleteOnClose, false);
        // 变更即时生效（US3 实时预览）：模式/显隐任一变化 → 主窗口重排
        connect(panelSettingsDialog_, &PanelSettingsDialog::configChanged, this,
                [this](const PanelLayoutConfig& cfg) { applyPanelLayout(cfg); });
    } else {
        // 打开时回显当前生效配置（US1）
        panelSettingsDialog_->setConfig(currentPanelLayoutConfig());
    }
    panelSettingsDialog_->show();
    panelSettingsDialog_->raise();
    panelSettingsDialog_->activateWindow();
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
        // 010-panel-layout-settings：嵌入式 PyShell（非全尺寸 *Only）属于中央区，
        // 全屏时一并隐藏（中央区占满整个主界面）
        if (consoleEmbeddedHost_) consoleEmbeddedHost_->hide();
    } else {
        int i = 0;
        for (QDockWidget* dock : {fileDock_, propertyDock_, pythonDock_}) {
            if (!dock) continue;
            if (i < docksVisibleBeforeFullscreen_.size())
                dock->setVisible(docksVisibleBeforeFullscreen_.at(i));
            ++i;
        }
        docksVisibleBeforeFullscreen_.clear();
        // 010-panel-layout-settings：嵌入式 PyShell 按当前布局配置恢复显隐
        if (consoleEmbeddedHost_ && consoleEmbeddedHost_->layout()->count() > 0) {
            consoleEmbeddedHost_->setVisible(lastPanelLayoutConfig_.consoleVisible);
        }
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
    // 009-supported-file-types：过滤串由文件类型权威目录派生（FR-006/FR-009-变更）；
    // 覆盖目录全量条目（含「规划中」.tdr/.dat/VTK 系列/.h5，供对话框发现与核对），
    // 规划中格式选中后由加载路径按「不支持」提示。单一事实来源：格式变更只改
    // src/core/io/file_type_catalog.h，此处禁手抄。
    const QString filter = [this]() {
        QStringList parts;
        for (const auto& g : perception::core::io::FileTypeCatalog::filterGroups()) {
            QStringList patterns;
            for (const auto& p : g.patterns) {
                patterns << QString::fromUtf8(p.c_str());
            }
            QString label;
            if (g.familyKey == "VTK") {
                label = tr("VTK Files");
            } else if (g.familyKey == "SVisual") {
                label = tr("SVisual Files");
            } else if (g.familyKey == "HDF5") {
                label = tr("HDF5 Files");
            } else if (g.familyKey == "Curve Data") {
                label = tr("Curve Data");
            } else {
                label = QString::fromUtf8(g.familyKey.c_str());
            }
            parts << tr("%1 (%2)").arg(label, patterns.join(QLatin1Char(' ')));
        }
        parts << tr("All Files (*)");
        return parts.join(QLatin1String(";;"));
    }();
    const QString file = runThemedFileDialog(this, tr("Open Data File"),
        QDir::currentPath(), filter, FileDialogMode::Open);
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
    // 005-multi-screen-maximize：最大化/还原几何管理已提取至 WindowMaximizeController
    //（006 重构）；按钮图标由 maximizedChanged 信号驱动刷新。
    if (e->type() == QEvent::WindowStateChange && windowMaximizeController_) {
        windowMaximizeController_->handleWindowStateChange();
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

    // 分界线（dock 分隔条）resize 拖拽高亮：命中检测/高亮/结束已提取至 DockDragOverlay
    // 控制器（006 重构）。press 在转发前检测命中（无拖拽、dock geometry 准确），不吞事件，
    // Qt 正常拖拽；release 在转发前结束高亮；move 在转发后同步高亮条（布局已更新，跟手）。
    if (e->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton && dockDragOverlay_) {
            const int hit = dockDragOverlay_->hitTest(me->pos());
            if (hit) dockDragOverlay_->beginSashDrag(hit);
        }
    } else if (e->type() == QEvent::MouseButtonRelease && dockDragOverlay_ &&
               dockDragOverlay_->isSashDragging()) {
        dockDragOverlay_->endSashDrag();
    }

    const bool handled = QMainWindow::event(e);

    if (dockDragOverlay_ && dockDragOverlay_->isSashDragging() &&
        e->type() == QEvent::MouseMove) {
        dockDragOverlay_->updateSashDrag();  // 转发后布局已更新 → 高亮条与真实分隔条同步
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
        if (msg->wParam == HTCAPTION && windowMaximizeController_) {
            windowMaximizeController_->toggleMaximize();  // 双击标题栏：最大化/还原
            return true;
        }
        break;
    case WM_GETMINMAXINFO:  // 005-multi-screen-maximize：最大化限制计算在控制器内完成
        if (windowMaximizeController_ &&
            windowMaximizeController_->handleWindowMessage(message, result)) {
            return true;
        }
        break;
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

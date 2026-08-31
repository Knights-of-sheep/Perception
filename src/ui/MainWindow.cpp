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



#include "ui/console/PythonConsole.h"
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

// 日志级别持久化 key（FR-013）：全局单一矩阵（FR-012 修订，不再区分控制台/文件）。
// 兼容：旧版本曾分别持久化 log/console/<LEVEL> 与 log/file/<LEVEL>；升级后优先读新
// 全局 key，未设置时回退到旧 log/console/<LEVEL>，保证既有用户设置不丢。
constexpr const char* kLogLevelPrefix         = "log/level/";
constexpr const char* kLogLegacyConsolePrefix = "log/console/";
constexpr const char* kLogVtkEnabled          = "log/vtkEnabled";
constexpr const char* kLogPathKey       = "log/path";  // 用户配置的日志文件路径（FR-016）

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

    sideLayout->addStretch();          // 顶部弹性间隔：Export 不再紧贴 top margin
    sideLayout->addWidget(exportBtn);
    sideLayout->addStretch();          // Export 与 Clear 之间弹性间隔
    sideLayout->addWidget(clearBtn);   // Clear 贴底（保留 bottom margin 8）
    sideLayout->addStretch(); 
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
void MainWindow::createCentralArea() {
    subwindowContainer_ = new SubwindowContainer(this);
    subwindowContainer_->setEmptyHint(
        tr("Drop VTK / SVisual / HDF5 data files here\nor press Ctrl+O to open"));
    setCentralWidget(subwindowContainer_);
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

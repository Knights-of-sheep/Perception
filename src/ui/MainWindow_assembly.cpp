// ===== MainWindow 装配函数（006-constitution-refactor 拆分）=====
// createActions / createMenus / createTitleBar / createToolbars / 动作图标注册
// 自 MainWindow.cpp 提取（MainWindow.cpp 专注窗口行为与事件，行数回落到红线内）。
// 这些函数是 MainWindow 的成员函数定义（跨 .cpp 文件，private 访问不受限），
// 共享依赖：makeActionIcon（本文件匿名 namespace）、showFramelessDialog、
// LogSettingsController、ThemeManager。
#include "ui/MainWindow.h"

#include "ui/action_icon_map.h"
#include "ui/console/PythonConsole.h"  // pythonConsole_->executeCommand（create_window 命令层）
#include "ui/frameless_dialog.h"
#include "ui/log/log_settings_controller.h"
#include "ui/maximize/window_maximize_controller.h"
#include "ui/theme/icon_factory.h"
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"
#include "ui/win_btn_icon.h"

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

namespace perception {
namespace ui {

namespace {
// 003-install-icon-bars：动作图标统一构造（IconFactory 五态派生，T-04/T-06）
QIcon makeActionIcon(const QString& iconId) {
    const auto* t = ThemeManager::current();
    return theme::IconFactory::actionIcon(iconId, t->colors.textWeak, t->colors.textOnSelection,
                                          t->colors.textDisabled, t->colors.accent);
}
}  // namespace

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
    setActionIcon(resetLayoutAction_, "view-layout-reset");

    // 004-dock-layout-manager：创建子窗口与布局设置（FR-001/002，US5 统一入口）
    newSubwindowAction_ = new QAction(tr("New Subwindow"), this);
    newSubwindowAction_->setStatusTip(tr("Create a new render subwindow"));
    setActionIcon(newSubwindowAction_, "view-add-subwindow");
    connect(newSubwindowAction_, &QAction::triggered, this, [this] {
        // FR-002/FR-027（plan Structure Decision）：菜单入口触发同一 create_window 命令行执行——
        // 动作槽构造无参 create_window() 命令文本提交 PythonConsole::executeCommand，
        // 命令文本回显、返回值（窗口 id）打印到 PyShell；title 缺省时自动 = id（Plot_N）。
        // 禁止绕过命令层直接调用 C++ 入口 createSubwindow。
        pythonConsole_->executeCommand(QStringLiteral("create_window()"));
    });

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
    setActionIcon(toggleFullscreenAction_, "view-fullscreen");
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

    // ---- 设置 → 日志（006-constitution-refactor：装配与逻辑在 LogSettingsController）----
    QMenu* settingsMenu = menuBar()->addMenu(tr("&Settings"));
    const LogMenuActions logMenu = logSettingsController_->attachLogMenu(settingsMenu);
    // 003：契约图标（动作仍由 MainWindow 注册，主题切换时随 iconItems_ 重建）
    setActionIcon(logMenu.vtkLog, "tools-log");
    setActionIcon(logMenu.openLogDir, "tools-log-dir");
    setActionIcon(logMenu.setLogPath, "tools-log-path");
    setActionIcon(logMenu.clearLog, "edit-delete-selection");

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
    connect(winMaxBtn_, &QToolButton::clicked, this, [this] {
        if (windowMaximizeController_) windowMaximizeController_->toggleMaximize();
    });
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

}  // namespace ui
}  // namespace perception

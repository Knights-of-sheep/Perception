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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "core/log/logger.h"

#include "ui/console/PythonConsole.h"
#include "ui/log/log_console_widget.h"
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"

namespace perception {
namespace ui {

namespace {
constexpr const char* kFileDockObjectName     = "fileDock";        // QSS 定位（ui-guidelines §4.1）
constexpr const char* kPropertyDockObjectName = "propertyDock";
constexpr const char* kPythonDockObjectName   = "pythonConsoleDock";  // 布局记忆用
constexpr const char* kLogDockObjectName      = "logDock";            // 布局记忆用
constexpr const char* kSettingsLayoutKey      = "mainWindow/layout";
constexpr const char* kSettingsGeometryKey    = "mainWindow/geometry";
constexpr const char* kEmptyTreeText          = "(尚未加载文件)";
constexpr const char* kEmptyPropertyText      = "(选择对象查看属性)";

// 日志级别持久化 key（FR-013）
constexpr const char* kLogConsolePrefix = "log/console/";
constexpr const char* kLogFilePrefix    = "log/file/";
constexpr const char* kLogVtkEnabled    = "log/vtkEnabled";
constexpr const char* kLogPathKey       = "log/path";  // 用户配置的日志文件路径（FR-016）

// ---- 自定义 Dock 标题栏 ----
// 背景：Qt5 + Fusion 风格下，浮动的 QDockWidget 不绘制 normal-button 子控件，
//       导致"恢复嵌入"按钮缺失，用户分离后找不到还原入口。
// 方案：setTitleBarWidget(DockTitleBar) 接管标题栏，三个按钮（分离/恢复嵌入/关闭）
//       全用显式 QToolButton，停靠态与浮动态均稳定可见；同时支持双击标题栏切换浮动。
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

        floatBtn_ = new QToolButton(this);
        floatBtn_->setObjectName(QStringLiteral("dockFloatBtn"));
        floatBtn_->setFixedSize(20, 20);
        floatBtn_->setCursor(Qt::PointingHandCursor);
        floatBtn_->setAutoRaise(true);
        floatBtn_->setFocusPolicy(Qt::NoFocus);

        closeBtn_ = new QToolButton(this);
        closeBtn_->setObjectName(QStringLiteral("dockCloseBtn"));
        closeBtn_->setFixedSize(20, 20);
        closeBtn_->setCursor(Qt::PointingHandCursor);
        closeBtn_->setAutoRaise(true);
        closeBtn_->setFocusPolicy(Qt::NoFocus);
        closeBtn_->setText(QString(QChar(0x2715)));   // ✕

        layout->addWidget(titleLabel_);
        layout->addWidget(floatBtn_);
        layout->addWidget(closeBtn_);

        titleLabel_->setText(dock_->windowTitle());
        refreshByState();

        connect(floatBtn_, &QToolButton::clicked, this, [this] {
            dock_->setFloating(!dock_->isFloating());
        });
        connect(closeBtn_, &QToolButton::clicked, dock_, &QDockWidget::close);
        connect(dock_, &QDockWidget::windowTitleChanged,
                titleLabel_, &QLabel::setText);
        connect(dock_, &QDockWidget::featuresChanged,
                this, [this](QDockWidget::DockWidgetFeatures f) {
            floatBtn_->setVisible(f & QDockWidget::DockWidgetFloatable);
            closeBtn_->setVisible(f & QDockWidget::DockWidgetClosable);
        });
        connect(dock_, &QDockWidget::topLevelChanged,
                this, &DockTitleBar::onTopLevelChanged);
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override {
        // 双击标题栏 = 切换浮动态（与默认 QDockWidget 行为一致，setTitleBarWidget 后
        // QDockWidget 的内置 mouseDoubleClickEvent 不再触发，需要我们手动接管）
        if (dock_->features() & QDockWidget::DockWidgetFloatable) {
            dock_->setFloating(!dock_->isFloating());
        }
        QWidget::mouseDoubleClickEvent(e);
    }

private:
    void refreshByState() {
        const auto f = dock_->features();
        floatBtn_->setVisible(f & QDockWidget::DockWidgetFloatable);
        closeBtn_->setVisible(f & QDockWidget::DockWidgetClosable);
        updateFloatIcon(dock_->isFloating());
    }
    void updateFloatIcon(bool floating) {
        // 浮动时按钮显示"恢复嵌入 ↩"；停靠时显示"分离 ⇅"
        floatBtn_->setText(QString(floating ? QChar(0x21A9) : QChar(0x21C5)));
        floatBtn_->setToolTip(floating ? tr("恢复嵌入") : tr("分离为浮动窗口"));
    }
    void onTopLevelChanged(bool topLevel) {
        updateFloatIcon(topLevel);
        if (topLevel) {
            // 浮动时把 QDockWidget 转成普通顶层窗口（默认是 Qt::Tool 工具窗）：
            // - 系统标题栏出现"最小化/最大化/关闭"按钮（Tool 窗默认只有关闭）
            // - 出现在任务栏，最小化后可从任务栏还原
            // - 可自由拖动边缘调整大小
            if (dock_->windowFlags().testFlag(Qt::Tool)) {
                dock_->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                                     Qt::WindowSystemMenuHint |
                                     Qt::WindowMinMaxButtonsHint |
                                     Qt::WindowCloseButtonHint);
                dock_->show();  // setWindowFlags 会隐藏窗口，需重新显示
            }
        } else {
            // 嵌入主窗口：复位窗口状态，避免残留最大化/最小化
            dock_->setWindowState(Qt::WindowNoState);
        }
    }

    QDockWidget*  dock_;
    QLabel*       titleLabel_;
    QToolButton*  floatBtn_;
    QToolButton*  closeBtn_;
};

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Perception"));
    resize(1360, 860);

    createActions();
    createMenus();
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

// ---- 动作 ----
void MainWindow::createActions() {
    openAction_ = new QAction(tr("打开(&O)..."), this);
    openAction_->setShortcut(QKeySequence::Open);  // Ctrl+O
    openAction_->setStatusTip(tr("打开 .plt / .csv 数据文件"));
    openAction_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(openAction_, &QAction::triggered, this, &MainWindow::openFile);

    exportAction_ = new QAction(tr("导出(&E)..."), this);
    exportAction_->setShortcut(QKeySequence::SaveAs);  // Ctrl+Shift+S
    exportAction_->setEnabled(false);  // M4 提供
    exportAction_->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(exportAction_, &QAction::triggered, this, [this] {
        statusBar()->showMessage(tr("导出功能将在后续版本提供"), 3000);
    });

    // 导出主界面图片（与"打开"同级，文件菜单；M3a 截图调试验证用）
    exportImageAction_ = new QAction(tr("导出主界面图片(&I)..."), this);
    exportImageAction_->setShortcut(QKeySequence("Ctrl+I"));
    exportImageAction_->setStatusTip(tr("将当前主窗口（含菜单/Dock/状态栏）保存为 PNG 图片"));
    connect(exportImageAction_, &QAction::triggered, this, &MainWindow::exportMainWindowImage);

    exitAction_ = new QAction(tr("退出(&X)"), this);
    exitAction_->setShortcut(QKeySequence::Quit);
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    toggleFileDockAction_ = new QAction(tr("数据集面板"), this);
    toggleFileDockAction_->setCheckable(true);
    toggleFileDockAction_->setShortcut(QKeySequence("Ctrl+1"));

    togglePropertyDockAction_ = new QAction(tr("属性面板"), this);
    togglePropertyDockAction_->setCheckable(true);
    togglePropertyDockAction_->setShortcut(QKeySequence("Ctrl+2"));

    togglePythonConsoleAction_ = new QAction(tr("Python 控制台"), this);
    togglePythonConsoleAction_->setCheckable(true);
    togglePythonConsoleAction_->setShortcut(QKeySequence("Ctrl+`"));

    toggleLogConsoleAction_ = new QAction(tr("日志输出(&L)"), this);
    toggleLogConsoleAction_->setCheckable(true);
    toggleLogConsoleAction_->setShortcut(QKeySequence("Ctrl+3"));
    toggleLogConsoleAction_->setStatusTip(tr("显示/隐藏统一日志输出面板（C++ / Python logging / Qt 消息）"));

    resetLayoutAction_ = new QAction(tr("重置布局"), this);
    resetLayoutAction_->setShortcut(QKeySequence("Ctrl+Shift+L"));
    resetLayoutAction_->setStatusTip(tr("恢复左侧数据集、右侧属性的默认布局"));
    resetLayoutAction_->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));

    aboutAction_ = new QAction(tr("关于(&A)..."), this);
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::about);
}

// ---- 菜单栏 ----
void MainWindow::createMenus() {
    // 禁用 Windows 原生菜单栏：否则菜单栏由系统绘制（浅色、不走 QSS），
    // 与深色主题割裂，窗口顶部会呈现"两层"的观感。
    menuBar()->setNativeMenuBar(false);

    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(openAction_);
    fileMenu->addAction(exportImageAction_);  // 导出主界面图片（与"打开"同级，调试快照）
    // exportAction_ 暂不加入菜单：M4 导出图表数据时再加回；避免与"导出主界面图片"混淆
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction_);

    QMenu* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    viewMenu->addAction(toggleFileDockAction_);
    viewMenu->addAction(togglePropertyDockAction_);
    viewMenu->addAction(togglePythonConsoleAction_);
    viewMenu->addAction(toggleLogConsoleAction_);
    viewMenu->addSeparator();
    viewMenu->addAction(resetLayoutAction_);

    // 主题（15 套，按 family 分组；勾选当前项，点击即热切换）
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

    // ---- 设置 → 日志级别（FR-012/013）----
    QMenu* settingsMenu = menuBar()->addMenu(tr("设置(&S)"));
    QMenu* logLevelMenu = settingsMenu->addMenu(tr("日志级别(&L)"));
    QMenu* consoleSub = logLevelMenu->addMenu(tr("控制台(&C)"));
    QMenu* fileSub = logLevelMenu->addMenu(tr("文件(&F)"));

    const char* const kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    // 批量开关：解决"单级别勾选状态不一致/用户找不到入口"的痛点。
    // 置于级别列表顶部，与具体级别分隔，语义直观。
    {
        consoleSub->addSeparator();
        consoleAllAction_ = consoleSub->addAction(tr("全部启用"));
        consoleNoneAction_ = consoleSub->addAction(tr("全部禁用"));
        connect(consoleAllAction_, &QAction::triggered, this, [this] { onConsoleAll(true); });
        connect(consoleNoneAction_, &QAction::triggered, this, [this] { onConsoleAll(false); });

        fileSub->addSeparator();
        fileAllAction_ = fileSub->addAction(tr("全部启用"));
        fileNoneAction_ = fileSub->addAction(tr("全部禁用"));
        connect(fileAllAction_, &QAction::triggered, this, [this] { onFileAll(true); });
        connect(fileNoneAction_, &QAction::triggered, this, [this] { onFileAll(false); });
    }

    for (int i = 0; i < 5; ++i) {
        QAction* c = consoleSub->addAction(QString::fromLatin1(kLevelNames[i]));
        c->setCheckable(true);
        c->setChecked(true);  // 默认矩阵：DEBUG 关、其余开
        if (i == 0) c->setChecked(false);
        c->setData(i);  // LogLevel 索引
        connect(c, &QAction::toggled, this, &MainWindow::onConsoleLevelToggled);
        consoleLevelActions_.append(c);

        QAction* f = fileSub->addAction(QString::fromLatin1(kLevelNames[i]));
        f->setCheckable(true);
        f->setChecked(true);
        if (i == 0) f->setChecked(false);
        f->setData(i);
        connect(f, &QAction::toggled, this, &MainWindow::onFileLevelToggled);
        fileLevelActions_.append(f);
    }

    // VTK 日志拦截开关（FR-011；VTK 未引入，仅配置项）
    vtkLogAction_ = settingsMenu->addAction(tr("VTK 日志拦截(&V)"));
    vtkLogAction_->setCheckable(true);
    vtkLogAction_->setChecked(true);  // 默认开启
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
    connect(openLogDirAction_, &QAction::triggered, this, &MainWindow::openLogDir);

    // 日志路径可配置（FR-016）：选择目录后迁移旧日志并持久化
    setLogPathAction_ = settingsMenu->addAction(tr("设置日志路径...(&P)"));
    setLogPathAction_->setEnabled(false);  // 路径注入前不可用
    connect(setLogPathAction_, &QAction::triggered, this, &MainWindow::setLogPath);
    // 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档
    clearLogAction_ = settingsMenu->addAction(tr("清除历史日志(&C)"));
    clearLogAction_->setEnabled(false);
    connect(clearLogAction_, &QAction::triggered, this, &MainWindow::clearLogHistory);

    QMenu* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(aboutAction_);
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

// ---- 日志级别（FR-012/013）----
void MainWindow::onConsoleLevelToggled(bool checked) {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;
    const int idx = act->data().toInt();
    const auto level = static_cast<perception::core::log::LogLevel>(idx);

    // 立即生效：找到已注册的控制台 sink 并更新其矩阵
    auto sink = perception::core::log::Logger::instance().findSink("ConsoleSink");
    if (sink) {
        sink->setLevelEnabled(level, checked);
    } else {
        // 之前静默返回——导致用户看到菜单勾上但无输出，以为"设置没用"。
        // 现写入 Logger，让"设置失败"可在日志/控制台中看到。
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            std::string("ConsoleSink not registered; toggling has no effect (level=")
            + perception::core::log::toString(level) + ")");
    }

    // 持久化
    QSettings settings;
    settings.setValue(QString::fromLatin1(kLogConsolePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                      checked);

    refreshLogStatusIndicator();
}

void MainWindow::onFileLevelToggled(bool checked) {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;
    const int idx = act->data().toInt();
    const auto level = static_cast<perception::core::log::LogLevel>(idx);

    auto sink = perception::core::log::Logger::instance().findSink("FileSink");
    if (sink) {
        sink->setLevelEnabled(level, checked);
    } else {
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            std::string("FileSink not registered; toggling has no effect (level=")
            + perception::core::log::toString(level) + ")");
    }

    QSettings settings;
    settings.setValue(QString::fromLatin1(kLogFilePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                      checked);

    refreshLogStatusIndicator();
}

// 控制台级别批量开关：解决"用户找不到单级别怎么开"和"QSettings 残留导致状态混乱"。
void MainWindow::onConsoleAll(bool enabled) {
    auto sink = perception::core::log::Logger::instance().findSink("ConsoleSink");
    if (!sink) {
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            "ConsoleSink not registered; batch toggle has no effect");
        return;
    }
    QSettings settings;
    for (int i = 0; i < consoleLevelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        // setChecked 不会触发 toggled（避免重入），需显式应用 + 持久化
        consoleLevelActions_[i]->setChecked(enabled);
        sink->setLevelEnabled(level, enabled);
        settings.setValue(QString::fromLatin1(kLogConsolePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                          enabled);
    }
    refreshLogStatusIndicator();
}

void MainWindow::onFileAll(bool enabled) {
    auto sink = perception::core::log::Logger::instance().findSink("FileSink");
    if (!sink) {
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            "FileSink not registered; batch toggle has no effect");
        return;
    }
    QSettings settings;
    for (int i = 0; i < fileLevelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        fileLevelActions_[i]->setChecked(enabled);
        sink->setLevelEnabled(level, enabled);
        settings.setValue(QString::fromLatin1(kLogFilePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                          enabled);
    }
}

// 状态栏实时显示 ConsoleSink 当前启用的级别——让"设置有没有用"一眼可验。
// 直接读 sink.matrix_（事实源），而非 action 的 checked（UI 状态），避免两边漂移。
void MainWindow::refreshLogStatusIndicator() {
    if (!consoleLogIndicator_) return;
    auto sink = perception::core::log::Logger::instance().findSink("ConsoleSink");
    if (!sink) {
        consoleLogIndicator_->setText(tr("控制台: 未注册"));
        return;
    }
    QStringList on;
    for (int i = 0; i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        if (sink->isLevelEnabled(level))
            on.append(QString::fromLatin1(perception::core::log::toString(level)));
    }
    if (on.isEmpty()) {
        consoleLogIndicator_->setText(tr("控制台: 无输出"));
    } else {
        consoleLogIndicator_->setText(tr("控制台: %1").arg(on.join(QLatin1Char('+'))));
    }
}

// 启动恢复：main.cpp 在 Logger::configure + addSink 后调用（FR-013）
void MainWindow::restoreLogSettings() {
    QSettings settings;
    for (int i = 0; i < consoleLevelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        const QString key = QString::fromLatin1(kLogConsolePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level));
        const bool on = settings.value(key, i != 0).toBool();  // 默认：DEBUG 关、其余开
        consoleLevelActions_[i]->setChecked(on);
        if (auto sink = perception::core::log::Logger::instance().findSink("ConsoleSink"))
            sink->setLevelEnabled(level, on);
    }
    for (int i = 0; i < fileLevelActions_.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        const QString key = QString::fromLatin1(kLogFilePrefix)
                          + QString::fromLatin1(perception::core::log::toString(level));
        const bool on = settings.value(key, i != 0).toBool();
        fileLevelActions_[i]->setChecked(on);
        if (auto sink = perception::core::log::Logger::instance().findSink("FileSink"))
            sink->setLevelEnabled(level, on);
    }
    if (vtkLogAction_) {
        const bool on = settings.value(QLatin1String(kLogVtkEnabled), true).toBool();
        vtkLogAction_->setChecked(on);
        // VTK 未引入：仅持久化开关；拦截桥随后续落地（FR-011）
    }

    refreshLogStatusIndicator();
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
    removeDockWidget(logDock_);
    QSettings().remove(kSettingsLayoutKey);  // 先清记忆，避免 restoreState 把 dock 再次隐藏
    addDockWidget(Qt::LeftDockWidgetArea, fileDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);
    addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);
    tabifyDockWidget(pythonDock_, logDock_);  // 日志面板与 Python 控制台同 tab 组
    // 强制可见+非浮动（旧 QSettings 状态可能让 dock 隐藏/浮动）
    fileDock_->setVisible(true);
    fileDock_->setFloating(false);
    propertyDock_->setVisible(true);
    propertyDock_->setFloating(false);
    pythonDock_->setVisible(true);
    pythonDock_->setFloating(false);
    logDock_->setVisible(true);   // 默认布局：日志面板可见（同 tab 组，可在 Python/Python 中切换）
    logDock_->setFloating(false);
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
    fileDock_->setWidget(fileTree_);
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
    propertyDock_->setWidget(propertyTree_);
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

    pythonDock_->setWidget(pythonContainer);
    pythonDock_->setTitleBarWidget(new DockTitleBar(pythonDock_));
    addDockWidget(Qt::BottomDockWidgetArea, pythonDock_);

    connect(togglePythonConsoleAction_, &QAction::triggered,
            this, [this](bool on) { pythonDock_->setVisible(on); });
    connect(pythonDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        togglePythonConsoleAction_->setChecked(visible);
    });
    togglePythonConsoleAction_->setChecked(true);

    // 底：日志输出面板（FR-006 修订——与 Python REPL 分离，C++/Python/Qt 日志
    // 统一投递到独立面板，不再混入 py shell 交互输出）。与 Python 控制台同 tab 组，
    // 默认隐藏：视图 → 日志输出（Ctrl+3）随时查看。
    logDock_ = new QDockWidget(tr("日志输出"), this);
    logDock_->setObjectName(kLogDockObjectName);
    logDock_->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    logDock_->setFeatures(QDockWidget::DockWidgetClosable |
                          QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable);
    logConsole_ = new LogConsoleWidget(logDock_);
    logDock_->setWidget(logConsole_);
    logDock_->setTitleBarWidget(new DockTitleBar(logDock_));
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);
    tabifyDockWidget(pythonDock_, logDock_);  // 与 Python 控制台同 tab 组
    logDock_->setVisible(true);               // 默认布局：日志面板与 Python 控制台同 tab 组可见
    pythonDock_->raise();                     // 默认激活 Python 控制台（日志面板可 Ctrl+3 切换）

    connect(toggleLogConsoleAction_, &QAction::triggered,
            this, [this](bool on) {
        if (on) { logDock_->setVisible(true); logDock_->raise(); }  // raise 激活该 tab
        else    { logDock_->setVisible(false); }
    });
    connect(logDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        toggleLogConsoleAction_->setChecked(visible);
    });

    // 重置布局
    connect(resetLayoutAction_, &QAction::triggered, this, &MainWindow::resetLayout);
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

    // 实时显示 ConsoleSink 启用的级别——一眼看出"控制台设置有没有用"。
    // 点击该按钮即打开日志面板（避免用户找不到 dock）。直接读 sink.matrix_ 而非
    // action 的 checked，保证状态栏与实际过滤行为一致（事实源）。
    consoleLogIndicator_ = new QToolButton(sb);
    consoleLogIndicator_->setToolTip(tr("点击打开日志输出面板"));
    consoleLogIndicator_->setAutoRaise(true);
    consoleLogIndicator_->setCursor(Qt::PointingHandCursor);
    connect(consoleLogIndicator_, &QToolButton::clicked, this, [this] {
        if (logDock_) {
            logDock_->setVisible(true);
            logDock_->raise();
            if (toggleLogConsoleAction_) toggleLogConsoleAction_->setChecked(true);
        }
    });
    sb->addPermanentWidget(consoleLogIndicator_);
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
           "<p>TCAD 数据可视化桌面工具（对标 svisual / TecplotSV）。</p>"
           "<p>技术栈：C++17 / Qt / VTK / pybind11</p>")
            .arg(QApplication::applicationVersion()));
}

}  // namespace ui
}  // namespace perception

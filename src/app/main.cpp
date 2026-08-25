#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFont>
#include <QIcon>
#include <QMouseEvent>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include "core/log/logger.h"
#include "core/log/terminal_sink.h"
#include "ui/MainWindow.h"
#include "ui/console/PythonConsole.h"
#include "ui/log/qt_message_bridge.h"
#include "ui/theme/theme_manager.h"

#ifndef PERCEPTION_APP_VERSION
#define PERCEPTION_APP_VERSION "0.1.0"
#endif

// Perception 桌面程序入口。
// 启动流程：高 DPI -> 应用元信息 -> 字体 -> 主题(QPalette+QSS) -> 主窗口。
int main(int argc, char* argv[])
{
    // 高 DPI 支持：须在 QApplication 构造前设置（Qt5 默认关闭）
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    // 003：静态库 perception_ui 中 theme.qrc 的资源符号需显式引用（MSVC /OPT:REF）
    Q_INIT_RESOURCE(theme);
    QApplication::setOrganizationName("Perception");
    QApplication::setApplicationName("Perception");
    QApplication::setApplicationVersion(PERCEPTION_APP_VERSION);

    // 工作目录：保持进程启动时的当前路径不变（从哪启动、QDir::current() 就是哪）。
    // 文件对话框（打开数据文件/导出图片/命令）默认目录与相对路径解析均以它为基准（FR-015）。

    // 切到 Fusion 风格：Windows 默认的 "windows" 风格会让 QMenuBar 等走系统原生绘制，
    // 即使 setNativeMenuBar(false) 也无法让 QSS 背景色生效（呈现"两层菜单栏"的浅色条）。
    // Fusion 是 Qt 自带的跨平台 style，QSS 100% 生效，是深色主题的标配。
    QApplication::setStyle("Fusion");

    // 字体层级：界面统一 Segoe UI 9pt（ui-guidelines §3.3）
    app.setFont(QFont(QStringLiteral("Segoe UI"), 9));

    // 主题：QPalette 兜底 + dark.qss 精修（ui-guidelines §4.2）
    perception::ui::ThemeManager::apply(app);

    perception::ui::MainWindow window;
    // 应用/窗口图标（图标设计规范 002-icon-design，A-03 交付物）：
    // app-icon.ico 注册于 theme.qrc /perception/icons，含 16..256 多分辨率（SC-006）。
    app.setWindowIcon(QIcon(QStringLiteral(":/perception/icons/icons/app/app-icon.ico")));
    window.setWindowIcon(app.windowIcon());
    window.show();
    // CONSOLE 子系统（终端显示日志）下，确保 GUI 主窗口不被终端窗口遮挡/抢占焦点
    window.raise();
    window.activateWindow();

    // ===== 统一日志装配（M4：统一日志模块）=====
    // 默认文件路径：%APPDATA%/Perception/logs/app.log（core 层不依赖 Qt，由 app 层计算传入）。
    // Windows 显式取 %APPDATA% 环境变量。实测/踩坑记录（Qt5.15）：
    //   AppDataLocation     -> %APPDATA%\<Org>\<App>（多一层 Perception，路径不符直觉）
    //   GenericDataLocation -> C:\ProgramData（系统级，普通用户不可写，mkpath 静默失败）
    //   GenericConfigLocation -> %LOCALAPPDATA%（与 %APPDATA% 混用会踩坑，已弃）
    // 其他平台用 GenericDataLocation（= $XDG_DATA_HOME 或 ~/.local/share）。
#ifdef Q_OS_WIN
    QString appData = QString::fromLocal8Bit(qgetenv("APPDATA"));
    if (appData.isEmpty())  // 极端环境（如非交互会话）兜底
        appData = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QString logDir = appData + QStringLiteral("/Perception/logs");
#else
    const QString logDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                         + QStringLiteral("/Perception/logs");
#endif
    QDir().mkpath(logDir);  // 确保目录存在（FileSink 亦会尝试创建，此处先行兜底）
    const QString defaultLogPath = logDir + QStringLiteral("/app.log");

    // 用户配置的日志路径覆盖默认值（设置菜单"设置日志路径..."，FR-016）：
    // 已保存则直接采用（路径无效时由 FileSink 降级处理，FR-005）
    const QString savedLogPath = QSettings().value(QLatin1String("log/path")).toString();
    const QString logPath = savedLogPath.isEmpty() ? defaultLogPath : savedLogPath;

    // 默认矩阵：DEBUG 关、INFO/WARN/ERROR/FATAL 开（FR-002）
    perception::core::log::LogLevelMatrix defaultMatrix;

    perception::core::log::Logger::Config cfg;
    cfg.filePath = logPath.toUtf8().toStdString();
    cfg.levelMatrix = defaultMatrix;
    cfg.vtkLoggingEnabled = true;      // FR-011 配置占位（VTK 未引入，拦截桥随后续落地）
    cfg.maxFileSize = 5 * 1024 * 1024; // 5MB
    cfg.maxBackups = 3;
    perception::core::log::Logger::instance().configure(cfg);
    window.setLogFilePath(logPath);  // 设置菜单展示路径 + 打开日志目录（FR-014）

    // 注册终端 sink：日志实时输出到进程终端（stdout/stderr，带级别颜色）。
    // 终端中运行 .\perception.exe 即在终端看到彩色日志；双击启动由系统分配控制台窗口。
    // 级别过滤与文件共用全局矩阵（FR-002），由下方 restoreLogSettings 统一设置。
    // GUI 内不再内置日志面板（2026-08-25 用户反馈：控制台=终端，嵌入面板不合理）。
    auto terminalSink = std::make_shared<perception::core::log::TerminalSink>();
    perception::core::log::Logger::instance().addSink(terminalSink);

    // 恢复 QSettings 中的日志级别矩阵与 VTK 开关（FR-013）
    window.restoreLogSettings();

    // 安装 Qt 消息重定向（FR-010）：qDebug/qWarning 等纳入统一日志流
    perception::ui::installQtMessageBridge();

    PERCEPTION_LOG_I("app started");

    // 调试参数（启动后抓图并自动退出，用于回归 UI 修复）：
    //   --snapshot <path>           默认布局抓主窗口
    //   --snapshot-float <path>     fileDock 分离（浮动）后，抓浮动窗口自身
    //   --snapshot-restore <path>   fileDock 分离 -> 模拟双击标题栏(toggleFloating) -> 抓主窗口
    //   --theme <id>                抓图前先切换主题（dark-classic / nord / ... 25 种）
    //   --snapshot-drag <zone>      抓图前模拟 Dock 拖拽：显示放置高亮（left/right/bottom/dock）
    //   --snapshot-focus <obj>      抓图前给指定 dock 设焦点，验证 focus rect 是否被抑制
    //   --snapshot-real-drag <dock> 用 QTest::mousePress/MouseMove/MouseRelease 模拟真实拖拽
    //                                （验证 DockTitleBar::eventFilter 拦截链是否生效）
    QString snapshotPath, snapshotFloatPath, snapshotRestorePath, snapshotThemeId,
            snapshotDragZone, snapshotFocusDock, snapshotRealDragDock, consoleScript;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("--snapshot") && i + 1 < argc) {
            snapshotPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--snapshot-float") && i + 1 < argc) {
            snapshotFloatPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--snapshot-restore") && i + 1 < argc) {
            snapshotRestorePath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--theme") && i + 1 < argc) {
            snapshotThemeId = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--snapshot-drag") && i + 1 < argc) {
            snapshotDragZone = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--snapshot-focus") && i + 1 < argc) {
            snapshotFocusDock = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--snapshot-real-drag") && i + 1 < argc) {
            snapshotRealDragDock = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--console-script") && i + 1 < argc) {
            consoleScript = QString::fromLocal8Bit(argv[++i]);
        }
    }
    const bool wantSnapshot = !snapshotPath.isEmpty() || !snapshotFloatPath.isEmpty()
                           || !snapshotRestorePath.isEmpty() || !snapshotDragZone.isEmpty()
                           || !snapshotFocusDock.isEmpty()
                           || !snapshotRealDragDock.isEmpty();
    if (wantSnapshot) {
        QTimer::singleShot(800, &window, [&window, snapshotPath, snapshotFloatPath,
                                          snapshotRestorePath, snapshotThemeId,
                                          snapshotDragZone, snapshotFocusDock,
                                          snapshotRealDragDock,
                                          consoleScript, &app] {
            QString savedThemeId;
            if (!snapshotThemeId.isEmpty()) {
                savedThemeId = perception::ui::ThemeManager::currentThemeId();
                window.applyTheme(snapshotThemeId);      // 先切主题（含 QSS+QPalette+勾选）
                QApplication::processEvents();
            }
            // 注入控制台脚本（验证内嵌 Python REPL 执行/输出）
            if (!consoleScript.isEmpty() && window.pythonConsole()) {
                window.pythonConsole()->runScript(consoleScript);
                QApplication::processEvents();
            }
            window.resetLayout();                        // 强制默认布局（不受 QSettings 记忆影响）
            QApplication::processEvents();
            // 分界线拖拽高亮：模拟按住 pythonConsoleDock 上缘（水平分隔条）拖动，
            // 验证真实分隔条（QDockWidgetSeparator）命中 + 高亮线长度=分隔条长度
            const QString themeIdNow = perception::ui::ThemeManager::currentThemeId();
            qInfo("sash-test: theme=%s window=(%d,%d,%d,%d)", themeIdNow.toUtf8().constData(),
                  window.x(), window.y(), window.width(), window.height());
            if (auto* sashDock =
                    window.findChild<QDockWidget*>(QStringLiteral("pythonConsoleDock"))) {
                const int sepY = sashDock->geometry().top();  // 水平分隔条（主窗口坐标）
                if (sepY > 0 && sepY < window.height()) {
                    const QPoint g1 = window.mapToGlobal(QPoint(window.width() / 2, sepY));
                    const QPoint g2 = window.mapToGlobal(QPoint(window.width() / 2, sepY));
                    QMouseEvent sashPress(QEvent::MouseButtonPress, window.mapFromGlobal(g1),
                                          g1, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(&window, &sashPress);
                    QApplication::processEvents();
                    QMouseEvent sashMv(QEvent::MouseMove, window.mapFromGlobal(g2), g2,
                                       Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(&window, &sashMv);
                    QApplication::processEvents();
                    QThread::msleep(120);  // 等待 overlay 重绘提交到屏幕合成
                    QApplication::processEvents();
                    // 验证高亮线"整条"：沿分隔条 y 采样 5 个 x（左/中左/中/中右/右），
                    // 每个点抓 3x3 中心像素，亮度 > 180 视为命中高亮线
                    int litCount = 0;
                    QColor centers[5];
                    const int xs[5] = {window.width() / 10, window.width() / 4,
                                       window.width() / 2, window.width() * 3 / 4,
                                       window.width() * 9 / 10};
                    for (int i = 0; i < 5; ++i) {
                        const QPoint gp = window.mapToGlobal(QPoint(xs[i], sepY));
                        const QPixmap px = QGuiApplication::primaryScreen()
                                               ->grabWindow(0, gp.x() - 1, gp.y() - 1, 3, 3);
                        if (px.isNull()) continue;
                        const QColor c = px.toImage().pixelColor(1, 1);
                        centers[i] = c;
                        if (qGray(c.rgb()) > 180) ++litCount;
                    }
                    qInfo("sash-test: horizontal sash lit=%d/5 x=[%d,%d,%d,%d,%d] colors=[#%02X%02X%02X,#%02X%02X%02X,#%02X%02X%02X,#%02X%02X%02X,#%02X%02X%02X]",
                          litCount, xs[0], xs[1], xs[2], xs[3], xs[4],
                          centers[0].red(), centers[0].green(), centers[0].blue(),
                          centers[1].red(), centers[1].green(), centers[1].blue(),
                          centers[2].red(), centers[2].green(), centers[2].blue(),
                          centers[3].red(), centers[3].green(), centers[3].blue(),
                          centers[4].red(), centers[4].green(), centers[4].blue());
                } else {
                    qInfo("sash-test: python dock top=%d invalid for window height=%d", sepY,
                          window.height());
                }
            }
            auto savePng = [](const QPixmap& pm, const QString& path) {
                if (pm.isNull() || path.isEmpty()) return;
                if (pm.save(path, "PNG")) {
                    qInfo("snapshot saved: %s", qPrintable(path));
                } else {
                    qWarning("snapshot save failed: %s", qPrintable(path));
                }
            };

            // 模拟 Dock 拖拽：在目标区域显示放置高亮后抓图（验证分割线高亮效果）
            if (!snapshotDragZone.isEmpty() && !snapshotPath.isEmpty()) {
                if (auto* dragDock = window.findChild<QDockWidget*>(QStringLiteral("fileDock"))) {
                    const QSize ws = window.size();
                    QPoint target;  // 主窗口坐标的目标放置点
                    if (snapshotDragZone == QLatin1String("right"))
                        target = QPoint(qRound(ws.width() * 0.92), ws.height() / 2);
                    else if (snapshotDragZone == QLatin1String("bottom"))
                        target = QPoint(ws.width() / 2, qRound(ws.height() * 0.92));
                    else if (snapshotDragZone == QLatin1String("dock"))
                        target = dragDock->geometry().center();  // 鼠标在 dock 内部（不在 zone 带内）
                    else
                        target = QPoint(qRound(ws.width() * 0.08), ws.height() / 2);
                    window.beginDockDrag(dragDock);
                    window.updateDockDrag(window.mapToGlobal(target));
                    QApplication::processEvents();
                }
            }

            // 让指定 dock 获得键盘焦点，验证 focus rect 是否被代理样式抑制
            if (!snapshotFocusDock.isEmpty()) {
                if (auto* d = window.findChild<QDockWidget*>(snapshotFocusDock)) {
                    d->setFocus();
                    QApplication::processEvents();
                }
            }

            // 真实拖拽测试：sendEvent 模拟标题栏拖拽，验证 DockTitleBar::eventFilter 拦截链。
            if (!snapshotRealDragDock.isEmpty() && !snapshotPath.isEmpty()) {
                if (auto* dock = window.findChild<QDockWidget*>(snapshotRealDragDock)) {
                    if (auto* tb = dock->titleBarWidget()) {
                        const QPoint pressLocal = tb->rect().center();
                        const QPoint pressGlobal = tb->mapToGlobal(pressLocal);
                        QMouseEvent pressEv(QEvent::MouseButtonPress, pressLocal,
                                            pressGlobal, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(tb, &pressEv);
                        QApplication::processEvents();
                        const QPoint moveGlobal = dock->mapToGlobal(QPoint(20, 20));
                        QMouseEvent mv(QEvent::MouseMove, tb->mapFromGlobal(moveGlobal),
                                       moveGlobal, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(tb, &mv);
                        QApplication::processEvents();
                    } else {
                        qWarning("real drag: dock %s has no titleBarWidget",
                                 qPrintable(snapshotRealDragDock));
                    }
                } else {
                    qWarning("real drag: dock %s not found", qPrintable(snapshotRealDragDock));
                }
            }

            if (!snapshotPath.isEmpty())
                savePng(window.grab(), snapshotPath);

            auto* fileDock = window.findChild<QDockWidget*>(QStringLiteral("fileDock"));
            if (fileDock) {
                // 分离：fileDock 浮动为独立窗口
                fileDock->setFloating(true);
                QApplication::processEvents();
                if (!snapshotFloatPath.isEmpty())
                    savePng(fileDock->grab(), snapshotFloatPath);  // 浮动窗口自身（应含"恢复嵌入"按钮）

                // 恢复：模拟双击标题栏（QDockWidget 双击内部即 setFloating(!isFloating())）
                fileDock->setFloating(!fileDock->isFloating());
                QApplication::processEvents();
                if (!snapshotRestorePath.isEmpty())
                    savePng(window.grab(), snapshotRestorePath);   // 主窗口（dock 已回位）
            }
            // 恢复原主题：快照不应污染用户持久化的主题设置
            if (!savedThemeId.isEmpty() && snapshotThemeId != savedThemeId) {
                perception::ui::ThemeManager::saveThemeId(savedThemeId);
            }
            app.quit();
        });
    }

    const int rc = app.exec();
    window.shutdownPython();  // 退出前释放内嵌 Python 运行时
    return rc;
}

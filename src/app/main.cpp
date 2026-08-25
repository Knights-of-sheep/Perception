#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFont>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>
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
    //   --theme <id>                抓图前先切换主题（dark-classic / nord / ... 15 种）
    QString snapshotPath, snapshotFloatPath, snapshotRestorePath, snapshotThemeId,
            consoleScript;
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
        } else if (arg == QLatin1String("--console-script") && i + 1 < argc) {
            consoleScript = QString::fromLocal8Bit(argv[++i]);
        }
    }
    const bool wantSnapshot = !snapshotPath.isEmpty() || !snapshotFloatPath.isEmpty()
                           || !snapshotRestorePath.isEmpty();
    if (wantSnapshot) {
        QTimer::singleShot(800, &window, [&window, snapshotPath, snapshotFloatPath,
                                          snapshotRestorePath, snapshotThemeId,
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
            auto savePng = [](const QPixmap& pm, const QString& path) {
                if (pm.isNull() || path.isEmpty()) return;
                if (pm.save(path, "PNG")) {
                    qInfo("snapshot saved: %s", qPrintable(path));
                } else {
                    qWarning("snapshot save failed: %s", qPrintable(path));
                }
            };

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

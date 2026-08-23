#include <QApplication>
#include <QDockWidget>
#include <QFont>
#include <QTimer>

#include "ui/MainWindow.h"
#include "ui/console/PythonConsole.h"
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
    QApplication::setOrganizationName("Perception");
    QApplication::setApplicationName("Perception");
    QApplication::setApplicationVersion(PERCEPTION_APP_VERSION);

    // 切到 Fusion 风格：Windows 默认的 "windows" 风格会让 QMenuBar 等走系统原生绘制，
    // 即使 setNativeMenuBar(false) 也无法让 QSS 背景色生效（呈现"两层菜单栏"的浅色条）。
    // Fusion 是 Qt 自带的跨平台 style，QSS 100% 生效，是深色主题的标配。
    QApplication::setStyle("Fusion");

    // 字体层级：界面统一 Segoe UI 9pt（ui-guidelines §3.3）
    app.setFont(QFont(QStringLiteral("Segoe UI"), 9));

    // 主题：QPalette 兜底 + dark.qss 精修（ui-guidelines §4.2）
    perception::ui::ThemeManager::apply(app);

    perception::ui::MainWindow window;
    window.show();

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

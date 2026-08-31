// ===== PyShell 分离后恢复对应当前模式交互单测（010-panel-layout-settings）=====
// 覆盖用户回归："*Only（非全尺寸）模式下 PyShell 分离（浮动）后 Re-dock，
//   应该恢复为当前选中模式的嵌入式窄条，而不是停在"底部全宽 dock"的初始布局形态
//   （默认 DualWithConsole 视觉）"。
//   1) *Only（DualOnly / DualReversedOnly）：嵌入式态 → 分离（全宽 dock+浮动）→
//      Re-dock（setFloating(false)）→ 自动收回嵌入式窄条（对应当前模式）；
//      且恢复后仍可再次展开（覆盖态不卡死）。
//   2) 全尺寸（*WithConsole）：正常 dock 分离/Re-dock 不触发自动收回（覆盖态不存在）。
// 需要 GUI 平台（QApplication + QtTest），Windows 桌面会话下运行。
#include "ui/MainWindow.h"
#include "ui/panellayout/panel_layout_config.h"

#include <QApplication>
#include <QDockWidget>
#include <QSettings>
#include <QTemporaryDir>
#include <QWidget>

#include <cstdio>

using perception::ui::PanelLayoutConfig;
using perception::ui::PanelLayoutMode;

namespace {

int checkRestoreToEmbedded(perception::ui::MainWindow& window, PanelLayoutMode mode,
                           const char* modeName) {
    PanelLayoutConfig cfg;
    cfg.mode = mode;
    window.applyPanelLayout(cfg);
    window.resize(1200, 800);
    window.show();
    QCoreApplication::processEvents();

    auto* host = window.findChild<QWidget*>(QStringLiteral("consoleEmbeddedHost"));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("pythonConsoleDock"));
    if (!host || !dock) {
        fprintf(stderr, "[%s] embedded host / python dock missing\n", modeName);
        return 1;
    }
    // 嵌入式态：宿主可见、dock 不可见
    if (!host->isVisible() || dock->isVisible()) {
        fprintf(stderr, "[%s] initial embedded state broken (host visible=%d dock visible=%d)\n",
                modeName, int(host->isVisible()), int(dock->isVisible()));
        return 2;
    }

    // 分离：展开为全尺寸底部 dock 并浮动
    window.setConsoleFullWidth(true);
    QCoreApplication::processEvents();
    if (!dock->isVisible()) {
        fprintf(stderr, "[%s] full-width expand failed (dock not visible)\n", modeName);
        return 3;
    }
    dock->setFloating(true);
    QCoreApplication::processEvents();
    if (!dock->isFloating()) {
        fprintf(stderr, "[%s] float (undock) failed\n", modeName);
        return 4;
    }

    // Re-dock：应自动收回嵌入式窄条（对应当前 *Only 模式，而非初始全宽 dock 形态）
    dock->setFloating(false);
    QCoreApplication::processEvents();
    if (!host->isVisible() || dock->isVisible()) {
        fprintf(stderr, "[%s] re-dock did NOT restore embedded host "
                        "(host visible=%d dock visible=%d)\n",
                modeName, int(host->isVisible()), int(dock->isVisible()));
        return 5;
    }

    // 恢复后可再次展开（覆盖态不卡死）
    window.setConsoleFullWidth(true);
    QCoreApplication::processEvents();
    if (!dock->isVisible()) {
        fprintf(stderr, "[%s] re-expand after restore failed\n", modeName);
        return 6;
    }
    window.setConsoleFullWidth(false);  // 再收回嵌入式
    QCoreApplication::processEvents();
    if (!host->isVisible() || dock->isVisible()) {
        fprintf(stderr, "[%s] final restore failed (host visible=%d dock visible=%d)\n",
                modeName, int(host->isVisible()), int(dock->isVisible()));
        return 7;
    }
    fprintf(stderr, "[%s] restore-to-mode ok\n", modeName);
    return 0;
}

// 全尺寸模式（*WithConsole）：PyShell 是正常底部 dock，分离/Re-dock 不得触发自动收回
int checkFullWidthUntouched(perception::ui::MainWindow& window, PanelLayoutMode mode,
                            const char* modeName) {
    PanelLayoutConfig cfg;
    cfg.mode = mode;
    window.applyPanelLayout(cfg);
    window.resize(1200, 800);
    window.show();
    QCoreApplication::processEvents();

    auto* host = window.findChild<QWidget*>(QStringLiteral("consoleEmbeddedHost"));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("pythonConsoleDock"));
    if (!host || !dock) return 1;
    if (!dock->isVisible() || host->isVisible()) {
        fprintf(stderr, "[%s] full-width initial state broken\n", modeName);
        return 2;
    }

    dock->setFloating(true);
    QCoreApplication::processEvents();
    dock->setFloating(false);
    QCoreApplication::processEvents();
    if (!dock->isVisible() || host->isVisible()) {
        fprintf(stderr, "[%s] full-width dock wrongly auto-collapsed to embedded\n", modeName);
        return 3;
    }
    fprintf(stderr, "[%s] full-width float/redock untouched ok\n", modeName);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("PerceptionTest"));
    app.setApplicationName(QStringLiteral("python_dock_restore_test"));

    // QSettings 隔离到临时目录：MainWindow 构造回读布局不污染真实用户配置
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        fprintf(stderr, "QTemporaryDir failed\n");
        return 1;
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());

    {
        perception::ui::MainWindow window;
        const int rc = checkRestoreToEmbedded(window, PanelLayoutMode::DualOnly, "DualOnly");
        if (rc != 0) return rc;
    }
    {
        perception::ui::MainWindow window;
        const int rc =
            checkRestoreToEmbedded(window, PanelLayoutMode::DualReversedOnly, "DualReversedOnly");
        if (rc != 0) return rc;
    }
    {
        perception::ui::MainWindow window;
        const int rc = checkFullWidthUntouched(
            window, PanelLayoutMode::DualWithConsole, "DualWithConsole");
        if (rc != 0) return rc;
    }
    {
        perception::ui::MainWindow window;
        const int rc = checkFullWidthUntouched(
            window, PanelLayoutMode::DualReversedWithConsole, "DualReversedWithConsole");
        if (rc != 0) return rc;
    }
    return 0;
}

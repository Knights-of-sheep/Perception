// 临时诊断：1) 四种模式弹窗预览截图；2) 四种模式实际主窗口布局截图
//（验证预览图与用户示意图一致；验证 PyShell 双宿主：*Only 嵌入式窄条 / *WithConsole 全宽 dock）
#include "ui/panellayout/panel_settings_dialog.h"
#include "ui/MainWindow.h"
#include "ui/theme/theme_manager.h"

#include <QApplication>
#include <QButtonGroup>
#include <QPushButton>

#include <cstdio>

using perception::ui::PanelLayoutConfig;
using perception::ui::PanelLayoutMode;
using perception::ui::PanelSettingsDialog;

static QPushButton* modeButton(PanelSettingsDialog& dialog, int id) {
    auto* group = dialog.findChild<QButtonGroup*>();
    if (!group) return nullptr;
    for (auto* btn : group->buttons()) {
        if (group->id(btn) == id) return qobject_cast<QPushButton*>(btn);
    }
    return nullptr;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    perception::ui::ThemeManager::apply(app);

    const char* names[] = {"dual_only", "dual_with_console", "dual_reversed_only",
                           "dual_reversed_with_console"};

    // ---- 弹窗预览图 ----
    {
        PanelSettingsDialog dialog{PanelLayoutConfig()};
        dialog.show();
        app.processEvents();
        for (int i = 0; i < 4; ++i) {
            auto* btn = modeButton(dialog, i);
            if (!btn) {
                fprintf(stderr, "mode button %d missing\n", i);
                return 1;
            }
            btn->click();
            app.processEvents();
            const QString out = QStringLiteral("E:/spec-work/Perception/bin/Release/panel_mode_%1.png")
                                    .arg(QLatin1String(names[i]));
            if (!dialog.grab().save(out)) {
                fprintf(stderr, "save failed: %s\n", qPrintable(out));
                return 2;
            }
            fprintf(stderr, "saved %s\n", qPrintable(out));
        }
    }

    // ---- 实际主窗口布局（PyShell 双宿主）----
    // 走真实信号通路：openPanelSettings 建立 configChanged → applyPanelLayout 连接，
    // 点击弹窗模式按钮驱动主窗口重排后抓图
    {
        perception::ui::MainWindow window;
        window.resize(1200, 800);
        window.show();
        window.openPanelSettings();
        app.processEvents();
        auto* dialog = window.findChild<PanelSettingsDialog*>();
        if (!dialog) {
            fprintf(stderr, "panel settings dialog not found\n");
            return 3;
        }
        for (int i = 0; i < 4; ++i) {
            auto* btn = modeButton(*dialog, i);
            if (!btn) {
                fprintf(stderr, "mode button %d missing\n", i);
                return 4;
            }
            btn->click();
            app.processEvents();
            const QString out = QStringLiteral("E:/spec-work/Perception/bin/Release/mainwin_mode_%1.png")
                                    .arg(QLatin1String(names[i]));
            if (!window.grab().save(out)) {
                fprintf(stderr, "main window save failed: %s\n", qPrintable(out));
                return 5;
            }
            fprintf(stderr, "saved %s\n", qPrintable(out));
        }
    }
    return 0;
}

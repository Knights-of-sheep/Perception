// ===== 面板布局设置对话框交互单测（010-panel-layout-settings）=====
// 覆盖（contracts/panel-settings-dialog.md §3/§4）：
//   - 初始回显（US1）：四模式按钮互斥勾选、三面板显隐勾选、预览 widget 存在
//   - 模式切换 → configChanged 负载正确（US1 场景 2 / US3 实时预览）
//   - 显隐 toggle → configChanged 负载正确（US2）
//   - 恢复默认 → configChanged(默认配置)（FR-002）
//   - setConfig 回显不发射 configChanged（打开回显，US1）
//   - OK → 持久化四 key（FR-007；QSettings 隔离到临时目录）
//   - Cancel → 恢复打开前快照并重放（US3 场景 3；控件同步回滚）
// 需要 GUI 平台（QApplication + QTest），Windows 桌面会话下运行。
//
// 注：configChanged 参数为自定义类型 PanelLayoutConfig。Qt 5.15 的 QSignalSpy 对该
// 类型参数无法反序列化为 QVariant（QMetaMethod::parameterMetaType 运行时查找失败，
// 实测 at(0) 为无效 QVariant），故改用 lambda 直连捕获负载（同线程同步调用，不经
// QVariant），计数与负载均可靠。
#include "ui/panellayout/panel_settings_dialog.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDir>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include <cstdio>

using perception::ui::PanelId;
using perception::ui::PanelLayoutConfig;
using perception::ui::PanelLayoutMode;
using perception::ui::PanelSettingsDialog;
using perception::ui::kPanelSettingsConsoleKey;
using perception::ui::kPanelSettingsDataKey;
using perception::ui::kPanelSettingsModeKey;
using perception::ui::kPanelSettingsPropertyKey;
using perception::ui::modeDefaultsConsoleVisible;

namespace {
// 按模式定位模式按钮（QButtonGroup id 0..3 = DualOnly/DualWithConsole/
// DualReversedOnly/DualReversedWithConsole，见 panel_settings_dialog.cpp）
QPushButton* modeButton(PanelSettingsDialog& dialog, int id) {
    auto* group = dialog.findChild<QButtonGroup*>();
    if (!group) return nullptr;
    for (auto* btn : group->buttons()) {
        if (group->id(btn) == id) return qobject_cast<QPushButton*>(btn);
    }
    return nullptr;
}

// configChanged 直连捕获器：记录发射次数与最后一次负载
struct ConfigRecorder {
    int count = 0;
    PanelLayoutConfig last;
    void capture(const PanelLayoutConfig& cfg) {
        last = cfg;
        ++count;
    }
};
}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // QSettings 隔离到临时目录：OK 持久化断言不污染真实用户配置（FR-007 测试）
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        fprintf(stderr, "QTemporaryDir failed\n");
        return 1;
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());

    // ---- 初始回显（US1 场景 1：对话框展示当前配置）----
    {
        PanelSettingsDialog dialog{PanelLayoutConfig()};
        dialog.show();
        QCoreApplication::processEvents();
        const auto buttons =
            dialog.findChildren<QPushButton*>(QStringLiteral("panelModeButton"));
        if (buttons.size() != 4) {
            fprintf(stderr, "expected 4 mode buttons, got %d\n", buttons.size());
            return 1;
        }
        int checkedCount = 0;
        for (auto* b : buttons) {
            if (b->isChecked()) ++checkedCount;
        }
        if (checkedCount != 1) {
            fprintf(stderr, "expected exactly 1 checked mode button\n");
            return 1;
        }
        auto* dataCheck = dialog.findChild<QCheckBox*>(QStringLiteral("panelDataVisibleCheck"));
        auto* propertyCheck =
            dialog.findChild<QCheckBox*>(QStringLiteral("panelPropertyVisibleCheck"));
        auto* consoleCheck =
            dialog.findChild<QCheckBox*>(QStringLiteral("panelConsoleVisibleCheck"));
        auto* preview = dialog.findChild<QWidget*>(QStringLiteral("panelPreviewWidget"));
        if (!dataCheck || !propertyCheck || !consoleCheck || !preview) {
            fprintf(stderr, "visibility checks or preview widget missing\n");
            return 1;
        }
        if (!dataCheck->isChecked() || !propertyCheck->isChecked() ||
            !consoleCheck->isChecked()) {
            fprintf(stderr, "default config should show all panels checked\n");
            return 1;
        }
    }

    // ---- 模式切换 → configChanged 负载正确（US1/US3）----
    {
        PanelSettingsDialog dialog{PanelLayoutConfig()};
        dialog.show();
        QCoreApplication::processEvents();
        ConfigRecorder rec;
        QObject::connect(&dialog, &PanelSettingsDialog::configChanged, &dialog,
                         [&rec](const PanelLayoutConfig& cfg) { rec.capture(cfg); });
        const PanelLayoutMode modes[] = {PanelLayoutMode::DualOnly,
                                         PanelLayoutMode::DualWithConsole,
                                         PanelLayoutMode::DualReversedOnly,
                                         PanelLayoutMode::DualReversedWithConsole};
        for (int i = 0; i < 4; ++i) {
            rec.count = 0;
            QPushButton* btn = modeButton(dialog, i);
            if (!btn) {
                fprintf(stderr, "mode button %d missing\n", i);
                return 1;
            }
            QTest::mouseClick(btn, Qt::LeftButton);  // 真实交互路径（clicked → idClicked）
            if (rec.count != 1) {
                fprintf(stderr, "mode switch #%d: expected 1 configChanged, got %d\n", i,
                        rec.count);
                return 1;
            }
            if (rec.last.mode != modes[i]) {
                fprintf(stderr, "mode switch #%d: expected mode %d got %d\n", i,
                        static_cast<int>(modes[i]), static_cast<int>(rec.last.mode));
                return 1;
            }
            if (!rec.last.dataVisible || !rec.last.propertyVisible) {
                fprintf(stderr, "mode switch #%d: data/property visibility should be unchanged\n", i);
                return 1;
            }
            if (rec.last.consoleVisible !=
                modeDefaultsConsoleVisible(modes[i])) {
                fprintf(stderr, "mode switch #%d: console visibility mismatch\n", i);
                return 1;
            }
        }
    }

    // ---- 显隐 toggle → configChanged 负载正确（US2 场景 1）----
    {
        PanelSettingsDialog dialog{PanelLayoutConfig()};
        dialog.show();
        QCoreApplication::processEvents();
        ConfigRecorder rec;
        QObject::connect(&dialog, &PanelSettingsDialog::configChanged, &dialog,
                         [&rec](const PanelLayoutConfig& cfg) { rec.capture(cfg); });
        auto* dataCheck = dialog.findChild<QCheckBox*>(QStringLiteral("panelDataVisibleCheck"));
        auto* propertyCheck =
            dialog.findChild<QCheckBox*>(QStringLiteral("panelPropertyVisibleCheck"));
        auto* consoleCheck =
            dialog.findChild<QCheckBox*>(QStringLiteral("panelConsoleVisibleCheck"));
        if (!dataCheck || !propertyCheck || !consoleCheck) return 1;

        dataCheck->setChecked(false);
        if (rec.count != 1) {
            fprintf(stderr, "data toggle: expected 1 configChanged\n");
            return 1;
        }
        if (rec.last.dataVisible || rec.last.mode != PanelLayoutMode::DualWithConsole) {
            fprintf(stderr, "data toggle payload mismatch\n");
            return 1;
        }
        propertyCheck->setChecked(false);
        consoleCheck->setChecked(false);
        if (rec.last.propertyVisible || rec.last.consoleVisible) {
            fprintf(stderr, "visibility toggle payload mismatch\n");
            return 1;
        }
        // 恢复勾选（expand 语义逆操作，US2 场景 3）
        dataCheck->setChecked(true);
        if (!rec.last.dataVisible) {
            fprintf(stderr, "restoring data visibility mismatch\n");
            return 1;
        }
    }

    // ---- 恢复默认 → configChanged(默认配置)（FR-002）----
    {
        PanelLayoutConfig initial;
        initial.mode = PanelLayoutMode::DualReversedOnly;
        initial.dataVisible = false;
        initial.propertyVisible = false;
        initial.consoleVisible = false;
        PanelSettingsDialog dialog(initial);
        dialog.show();
        QCoreApplication::processEvents();
        ConfigRecorder rec;
        QObject::connect(&dialog, &PanelSettingsDialog::configChanged, &dialog,
                         [&rec](const PanelLayoutConfig& cfg) { rec.capture(cfg); });
        auto* restore = dialog.findChild<QPushButton*>(QStringLiteral("panelResetDefaultsButton"));
        if (!restore) {
            fprintf(stderr, "reset-defaults button missing\n");
            return 1;
        }
        QTest::mouseClick(restore, Qt::LeftButton);
        if (rec.count != 1) {
            fprintf(stderr, "restore defaults: expected 1 configChanged\n");
            return 1;
        }
        if (rec.last.mode != PanelLayoutMode::DualWithConsole || !rec.last.dataVisible ||
            !rec.last.propertyVisible || !rec.last.consoleVisible) {
            fprintf(stderr, "restore defaults payload mismatch\n");
            return 1;
        }
    }

    // ---- setConfig 回显不发射 configChanged（打开回显，US1 场景 1）----
    {
        PanelSettingsDialog dialog{PanelLayoutConfig()};
        ConfigRecorder rec;
        QObject::connect(&dialog, &PanelSettingsDialog::configChanged, &dialog,
                         [&rec](const PanelLayoutConfig& cfg) { rec.capture(cfg); });
        PanelLayoutConfig cfg;
        cfg.mode = PanelLayoutMode::DualReversedOnly;
        cfg.dataVisible = false;
        dialog.setConfig(cfg);
        if (rec.count != 0) {
            fprintf(stderr, "setConfig must not emit configChanged\n");
            return 1;
        }
        if (dialog.current().mode != PanelLayoutMode::DualReversedOnly ||
            dialog.current().dataVisible) {
            fprintf(stderr, "setConfig did not sync controls\n");
            return 1;
        }
    }

    // ---- OK → 持久化四 key（FR-007，US1 场景 3）----
    {
        PanelLayoutConfig cfg;
        cfg.mode = PanelLayoutMode::DualReversedWithConsole;
        cfg.dataVisible = true;
        cfg.propertyVisible = false;
        cfg.consoleVisible = true;
        PanelSettingsDialog dialog(cfg);
        dialog.show();
        QCoreApplication::processEvents();
        auto* okBtn = dialog.findChild<QPushButton*>(QStringLiteral("panelOkButton"));
        if (!okBtn) {
            fprintf(stderr, "OK button missing\n");
            return 1;
        }
        QTest::mouseClick(okBtn, Qt::LeftButton);
        if (dialog.result() != QDialog::Accepted) {
            fprintf(stderr, "OK click did not accept dialog\n");
            return 1;
        }
        QSettings s;
        if (s.value(QLatin1String(kPanelSettingsModeKey)).toString() !=
            QStringLiteral("DualReversedWithConsole")) {
            fprintf(stderr, "mode key not persisted\n");
            return 1;
        }
        if (s.value(QLatin1String(kPanelSettingsDataKey)).toBool() != true) {
            fprintf(stderr, "dataVisible key not persisted\n");
            return 1;
        }
        if (s.value(QLatin1String(kPanelSettingsPropertyKey)).toBool() != false) {
            fprintf(stderr, "propertyVisible key not persisted\n");
            return 1;
        }
        if (s.value(QLatin1String(kPanelSettingsConsoleKey)).toBool() != true) {
            fprintf(stderr, "consoleVisible key not persisted\n");
            return 1;
        }
    }

    // ---- Cancel → 恢复打开前快照并重放（US3 场景 3）----
    {
        PanelLayoutConfig initial;
        initial.mode = PanelLayoutMode::DualReversedOnly;
        initial.dataVisible = false;
        initial.propertyVisible = true;
        initial.consoleVisible = false;
        PanelSettingsDialog dialog(initial);
        dialog.show();
        QCoreApplication::processEvents();
        ConfigRecorder rec;
        QObject::connect(&dialog, &PanelSettingsDialog::configChanged, &dialog,
                         [&rec](const PanelLayoutConfig& cfg) { rec.capture(cfg); });
        auto* dataCheck = dialog.findChild<QCheckBox*>(QStringLiteral("panelDataVisibleCheck"));
        auto* cancelBtn = dialog.findChild<QPushButton*>(QStringLiteral("panelCancelButton"));
        if (!dataCheck || !cancelBtn) {
            fprintf(stderr, "data check or cancel button missing\n");
            return 1;
        }
        dataCheck->setChecked(true);  // 先产生一次预览变更
        rec.count = 0;
        QTest::mouseClick(cancelBtn, Qt::LeftButton);
        if (rec.count < 1) {
            fprintf(stderr, "Cancel did not re-emit snapshot configChanged\n");
            return 1;
        }
        if (rec.last.mode != initial.mode || rec.last.dataVisible != initial.dataVisible ||
            rec.last.propertyVisible != initial.propertyVisible ||
            rec.last.consoleVisible != initial.consoleVisible) {
            fprintf(stderr, "Cancel snapshot payload mismatch\n");
            return 1;
        }
        // 控件同步回滚（对话框内部状态）
        if (dialog.current().mode != initial.mode || dialog.current().dataVisible ||
            !dialog.current().propertyVisible || dialog.current().consoleVisible) {
            fprintf(stderr, "controls not rolled back after Cancel\n");
            return 1;
        }
    }

    puts("panel_settings_dialog_test: ALL PASS");
    return 0;
}

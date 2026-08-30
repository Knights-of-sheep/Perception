#include "ui/theme/theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>

#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_dialog_layer.h"

namespace perception {
namespace ui {

// ===== 启动入口：应用上次保存的主题 =====
void ThemeManager::apply(QApplication& app) {
    applyTheme(currentThemeId(), app);
}

// ===== 热切换 =====
void ThemeManager::applyTheme(const QString& themeId, QApplication& app) {
    const theme::ThemeDescriptor* t = find(themeId);

    // 1. 风格：Fusion（跨平台绘制一致 + QSS 100% 生效，ui-guidelines §4.2）
    app.setStyle(QStyleFactory::create("Fusion"));

    // 2. QPalette 兜底：覆盖非 QSS 控件（原生绘制路径）
    applyPalette(app, t->colors);

    // 3. QSS 精修：模板 + 当前色板 -> 全量样式（一次 setStyleSheet 重建样式树）
    app.setStyleSheet(renderQss(*t));

    saveThemeId(t->id);
}

// ===== 目录访问 =====
const theme::ThemeDescriptor* ThemeManager::themes() { return theme::kThemes; }
int ThemeManager::themeCount() { return theme::kThemeCount; }

const theme::ThemeDescriptor* ThemeManager::find(const QString& id) {
    return theme::findTheme(id);
}

const theme::ThemeDescriptor* ThemeManager::current() {
    return find(currentThemeId());
}

// ===== 持久化 =====
QString ThemeManager::currentThemeId() {
    return find(QSettings().value(theme::kSettingsThemeKey).toString())->id;
}

void ThemeManager::saveThemeId(const QString& id) {
    QSettings().setValue(theme::kSettingsThemeKey, id);
}

// ===== QPalette 兜底（ui-guidelines §4.2：disabled 态最易漏）=====
void ThemeManager::applyPalette(QApplication& app, const theme::ThemeColors& c) {
    QPalette p;
    p.setColor(QPalette::Window,          c.windowBg);
    p.setColor(QPalette::WindowText,      c.text);
    p.setColor(QPalette::Base,            c.controlBg);
    p.setColor(QPalette::AlternateBase,   c.panelBg);
    p.setColor(QPalette::Text,            c.text);
    p.setColor(QPalette::PlaceholderText, c.textWeak);
    p.setColor(QPalette::Button,          c.controlBg);
    p.setColor(QPalette::ButtonText,      c.text);
    p.setColor(QPalette::BrightText,      c.white);
    p.setColor(QPalette::Highlight,       c.accent);
    p.setColor(QPalette::HighlightedText, c.textOnAccent);
    p.setColor(QPalette::ToolTipBase,     c.controlBg);
    p.setColor(QPalette::ToolTipText,     c.text);

    // Disabled 组
    p.setColor(QPalette::Disabled, QPalette::WindowText,   c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::Text,         c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText,   c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, c.textDisabled);

    app.setPalette(p);
}

// ===== QSS 渲染：模板 -> 当前主题（token 一一替换）=====
QString ThemeManager::renderQss(const theme::ThemeDescriptor& t) {
    QFile file(QStringLiteral(":/perception/theme/theme_template.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QString qss = QString::fromUtf8(file.readAll());

    const theme::ThemeColors& c = t.colors;

    // 弹窗背景层：语义色优先（供未来定制钩子），否则派生（008-unify-dialog-styling）；
    // High Contrast family 派生返回无效色 → 回退 windowBg（层次靠 QSS 1px 边框）
    QColor dialogBg = c.dialogBg;
    if (!dialogBg.isValid()) {
        dialogBg = theme::deriveDialogBg(c.windowBg, c.text, t.family);
        if (!dialogBg.isValid()) dialogBg = c.windowBg;
    }

    struct Token { const char* name; QColor color; };
    const Token tokens[] = {
        {"@windowBg@", c.windowBg}, {"@panelBg@", c.panelBg},
        {"@controlBg@", c.controlBg}, {"@viewBg@", c.viewBg},
        {"@dialogBg@", dialogBg},
        {"@text@", c.text}, {"@textWeak@", c.textWeak},
        {"@textDisabled@", c.textDisabled},
        {"@border@", c.border}, {"@borderWeak@", c.borderWeak},
        {"@accent@", c.accent}, {"@accentHover@", c.accentHover},
        {"@accentPressed@", c.accentPressed}, {"@selectionBg@", c.selectionBg},
        {"@hoverBg@", c.hoverBg}, {"@surfaceElev@", c.surfaceElev},
        {"@itemHover@", c.itemHover}, {"@buttonHover@", c.buttonHover},
        {"@handleHover@", c.handleHover},
        {"@dockTitleBg@", c.dockTitleBg},
        {"@dangerText@", c.dangerText}, {"@dangerHoverBg@", c.dangerHoverBg},
        {"@success@", c.success}, {"@warning@", c.warning}, {"@danger@", c.danger},
        {"@white@", c.white},
        {"@textOnSelection@", c.textOnSelection},
        {"@textOnAccent@", c.textOnAccent},
    };
    for (const Token& t : tokens) {
        qss.replace(QLatin1String(t.name), t.color.name());
    }
    return qss;
}

}  // namespace ui
}  // namespace perception

// ===== ThemeManager：主题应用与热切换 =====
// 架构：theme_template.qss（@token@ 占位符）+ theme_catalog.h（25 套色板）
//       -> 运行时渲染出最终 QSS；QPalette 兜底（ui-guidelines §4.2/§4.5/§7.5）。
#pragma once

#include <QString>

class QApplication;

namespace perception {
namespace ui {
namespace theme {
struct ThemeColors;
struct ThemeDescriptor;
}  // namespace theme

class ThemeManager {
public:
    // 启动入口：读取 QSettings 上次主题并应用（main() 中调用一次）
    static void apply(QApplication& app);
    // 热切换：按主题 id 渲染 QSS + 应用 QPalette + 持久化
    static void applyTheme(const QString& themeId, QApplication& app);

    // 目录访问
    static const theme::ThemeDescriptor* themes();   // 主题数组（25）
    static int themeCount();
    static const theme::ThemeDescriptor* find(const QString& id);  // 无效 id 兜底默认
    static const theme::ThemeDescriptor* current();                // 按 QSettings 取当前

    // 持久化
    static QString currentThemeId();
    static void saveThemeId(const QString& id);

private:
    static void applyPalette(QApplication& app, const theme::ThemeColors& colors);
    static QString renderQss(const theme::ThemeDescriptor& theme);  // 模板 -> QSS
};

}  // namespace ui
}  // namespace perception

// ===== Perception 主题目录：25 套主题色板（唯一语义色定义）=====
// 架构：QSS 模板（theme_template.qss）含 @token@ 占位符，
//       切换主题时 ThemeManager 用本文件色板逐 token 渲染，得到最终 QSS。
// 规范：docs/design/ui-guidelines.md §4.5（Token 单点定义）。
// 006-constitution-refactor：色板按明暗/对比分组拆分至
//   theme_catalog_dark.h（15 深色）/ theme_catalog_light.h（10 浅色）/
//   theme_catalog_hc.h（3 高对比），结构定义移至 theme_types.h（避免循环依赖）；
//   本文件保留 目录 / 常量 / 查询（单一职责，头文件行数回落红线内）。
// 注意：每套主题需通过 build/_theme_check.py 的 WCAG 对比度校验
//       （text/textWeak/textDisabled/textOnSelection/textOnAccent 与各背景层级）。
#pragma once

#include <QString>

#include "theme_catalog_dark.h"
#include "theme_catalog_hc.h"
#include "theme_catalog_light.h"

namespace perception {
namespace ui {
namespace theme {

// 主题目录（顺序即菜单顺序：深色 → 浅色 → 高对比）
inline const ThemeDescriptor kThemes[] = {
    {"dark-classic",      "Dark Classic",        "Dark",   kDarkClassic},
    {"dark-blue",         "Deep Blue",           "Dark",   kDarkBlue},
    {"nord",              "Nord",                "Dark",   kNord},
    {"one-dark",          "One Dark",            "Dark",   kOneDark},
    {"dracula",           "Dracula",             "Dark",   kDracula},
    {"monokai",           "Monokai",             "Dark",   kMonokai},
    {"gruvbox-dark",      "Gruvbox Dark",        "Dark",   kGruvboxDark},
    {"solarized-dark",    "Solarized Dark",      "Dark",   kSolarizedDark},
    {"tokyo-night",       "Tokyo Night",         "Dark",   kTokyoNight},
    {"rose-pine",         "Rosé Pine",           "Dark",   kRosePine},
    {"catppuccin-mocha",  "Catppuccin Mocha",    "Dark",   kCatppuccinMocha},
    {"everforest-dark",   "Everforest Dark",     "Dark",   kEverforestDark},
    {"kanagawa",          "Kanagawa",            "Dark",   kKanagawa},
    {"night-owl",         "Night Owl",           "Dark",   kNightOwl},
    {"ayu-dark",          "Ayu Dark",            "Dark",   kAyuDark},
    {"light-classic",     "Light Classic",       "Light",  kLightClassic},
    {"light-blue",        "Light Blue",          "Light",  kLightBlue},
    {"material-light",    "Material Light",      "Light",  kMaterialLight},
    {"solarized-light",   "Solarized Light",     "Light",  kSolarizedLight},
    {"rose-pine-dawn",    "Rosé Pine Dawn",      "Light",  kRosePineDawn},
    {"catppuccin-latte",  "Catppuccin Latte",    "Light",  kCatppuccinLatte},
    {"github-light",      "GitHub Light",        "Light",  kGithubLight},
    {"hc-black",          "High Contrast Black", "High Contrast", kHighContrastBlack},
    {"hc-white",          "High Contrast White", "High Contrast", kHighContrastWhite},
    {"hc-blue",           "High Contrast Blue",  "High Contrast", kHighContrastBlue},
};
inline constexpr int kThemeCount = 25;
inline constexpr const char* kDefaultThemeId = "dark-classic";
inline constexpr const char* kSettingsThemeKey = "theme/id";

inline const ThemeDescriptor* findTheme(const QString& id) {
    for (const auto& t : kThemes) {
        if (id == QLatin1String(t.id)) return &t;
    }
    return &kThemes[0];  // 兜底：默认深色经典
}

}  // namespace theme
}  // namespace ui
}  // namespace perception

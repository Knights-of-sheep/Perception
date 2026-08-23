// ===== Perception 主题目录：15 套主题色板（唯一语义色定义）=====
// 架构：QSS 模板（theme_template.qss）含 @token@ 占位符，
//       切换主题时 ThemeManager 用本文件色板逐 token 渲染，得到最终 QSS。
//       色值来源：经典配色（VS Code / Nord / Dracula / One Dark / Monokai /
//       Gruvbox / Solarized / Tokyo Night / Material）官方色板，色盲友好序沿用 Okabe-Ito。
// 规范：docs/design/ui-guidelines.md §4.5（Token 单点定义）。
#pragma once

#include <QColor>
#include <QString>

namespace perception {
namespace ui {
namespace theme {

// 语义 token：与 theme_template.qss 占位符一一对应（小驼峰，@name@ 形式）
struct ThemeColors {
    // ---- 分层背景 ----
    QColor windowBg;    // BG_WINDOW    主窗口背景
    QColor panelBg;     // BG_PANEL     菜单栏 / 工具栏 / Dock / 列表底
    QColor controlBg;   // BG_CONTROL   按钮 / 输入框 / 菜单弹出底
    QColor viewBg;      // BG_VIEW      中央视图底（绘图区）
    // ---- 前景与边框 ----
    QColor text;           // FG_TEXT          主文字
    QColor textWeak;       // FG_TEXT_WEAK     次要文字
    QColor textDisabled;   // FG_TEXT_DISABLED 禁用文字
    QColor border;         // BORDER           边框 / 分隔线
    QColor borderWeak;     // BORDER_WEAK      弱分隔线
    // ---- 强调与状态 ----
    QColor accent;          // ACCENT       全 UI 主色（焦点环 / primary 按钮 / 滚动条按压）
    QColor accentHover;     // ACCENT_HOVER
    QColor accentPressed;   // ACCENT_PRESSED
    QColor selectionBg;     // SELECTION_BG 列表 / 树选中底
    QColor hoverBg;         // 通用 hover 底（菜单项 / 工具栏按钮）
    QColor surfaceElev;     // 抬升面（表头 / 交替行 / Tab 未选中 / disabled 按钮底）
    QColor itemHover;       // 树 / 列表 item hover
    QColor buttonHover;     // QPushButton hover
    QColor handleHover;     // 滚动条 handle hover
    QColor dockTitleBg;     // 自定义 Dock 标题栏底
    QColor dangerText;      // 关闭按钮文字
    QColor dangerHoverBg;   // 关闭按钮 hover 底
    // ---- 状态色 ----
    QColor success;   // SUCCESS
    QColor warning;   // WARNING
    QColor danger;    // DANGER
    // ---- 文字（随明暗翻转）----
    QColor white;           // 纯白 / 亮文字（primary 按钮文字等）
    QColor textOnSelection; // 选中底上的文字（浅色主题用深色，深色主题用白）
    QColor textOnAccent;    // accent 底上的文字
};

struct ThemeDescriptor {
    const char* id;     // 持久化 id（QSettings）
    const char* name;   // 菜单显示名
    const char* family; // 类别：深色 / 浅色 / 高对比（菜单分组用）
    ThemeColors colors;
};

// ===== 深色主题（9）=====

// 1. 深色经典：产品默认（vsCode Dark+ 系）
inline const ThemeColors kDarkClassic = {
    {"#1E1E1E"}, {"#252526"}, {"#3C3C3C"}, {"#161616"},
    {"#D4D4D4"}, {"#9D9D9D"}, {"#6E6E6E"},
    {"#454545"}, {"#3F3F3F"},
    {"#0A84FF"}, {"#3399FF"}, {"#0066CC"}, {"#094771"},
    {"#3C3C3C"}, {"#2A2A2A"}, {"#333333"}, {"#4A4A4A"}, {"#5A5A5A"},
    {"#2D2D30"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#4EC9B0"}, {"#CCA700"}, {"#F14C4C"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
};

// 2. 深蓝：冷调蓝灰（海洋 / 数据监视风格）
inline const ThemeColors kDarkBlue = {
    {"#15202B"}, {"#1B2838"}, {"#2B3A4B"}, {"#101822"},
    {"#D8E1EA"}, {"#93A3B4"}, {"#5D6B7A"},
    {"#3A4A5D"}, {"#334252"},
    {"#2E8FFF"}, {"#5AA8FF"}, {"#1A6FD0"}, {"#123E6B"},
    {"#2B3A4B"}, {"#202E3D"}, {"#32445A"}, {"#35465A"}, {"#40556C"},
    {"#223243"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#4EC9B0"}, {"#CCA700"}, {"#F14C4C"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
};

// 3. 北欧 Nord：官方北极蓝（nordtheme.com，frost/aurora）
inline const ThemeColors kNord = {
    {"#2E3440"}, {"#3B4252"}, {"#434C5E"}, {"#272C36"},
    {"#D8DEE9"}, {"#98A2B3"}, {"#5E6A7A"},
    {"#4C566A"}, {"#434C5E"},
    {"#88C0D0"}, {"#A3D3DF"}, {"#5E81AC"}, {"#434C5E"},
    {"#434C5E"}, {"#3B4252"}, {"#4C566A"}, {"#4C566A"}, {"#5E81AC"},
    {"#3B4252"}, {"#BF616A"}, {"#5A2E33"},
    {"#A3BE8C"}, {"#EBCB8B"}, {"#BF616A"},
    {"#ECEFF4"}, {"#ECEFF4"}, {"#2E3440"},
};

// 4. One Dark：Atom / VS Code 默认深色
inline const ThemeColors kOneDark = {
    {"#21252B"}, {"#282C34"}, {"#323842"}, {"#1B1E24"},
    {"#ABB2BF"}, {"#7F8899"}, {"#5A6270"},
    {"#3A4048"}, {"#31363E"},
    {"#61AFEF"}, {"#82C2F2"}, {"#3E8FD9"}, {"#264F78"},
    {"#323842"}, {"#2C313A"}, {"#3A4048"}, {"#3A4048"}, {"#4B5263"},
    {"#2C313A"}, {"#E06C75"}, {"#5A2A2F"},
    {"#98C379"}, {"#E5C07B"}, {"#E06C75"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
};

// 5. 德古拉 Dracula：draculatheme.com 官方
inline const ThemeColors kDracula = {
    {"#21222C"}, {"#282A36"}, {"#343746"}, {"#191A24"},
    {"#F8F8F2"}, {"#9096A6"}, {"#5B6274"},
    {"#44475A"}, {"#3A3D4D"},
    {"#BD93F9"}, {"#CEAEFB"}, {"#9A6FE3"}, {"#44475A"},
    {"#343746"}, {"#2F3140"}, {"#3E4152"}, {"#3E4152"}, {"#4A4E60"},
    {"#2F3140"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#50FA7B"}, {"#F1FA8C"}, {"#FF5555"},
    {"#FFFFFF"}, {"#F8F8F2"}, {"#1E1E2E"},
};

// 6. Monokai：Sublime Text 经典
inline const ThemeColors kMonokai = {
    {"#1F201C"}, {"#272822"}, {"#33352E"}, {"#181913"},
    {"#F8F8F2"}, {"#A0A096"}, {"#65665D"},
    {"#3E4038"}, {"#33352E"},
    {"#A6E22E"}, {"#BCE55C"}, {"#85B41F"}, {"#49483E"},
    {"#33352E"}, {"#2C2E27"}, {"#3E4038"}, {"#3E4038"}, {"#4A4C42"},
    {"#2C2E27"}, {"#F92672"}, {"#5A1F2E"},
    {"#A6E22E"}, {"#E6DB74"}, {"#F92672"},
    {"#FFFFFF"}, {"#F8F8F2"}, {"#1E1E1E"},
};

// 7. 格鲁夫深色 Gruvbox：morhetz 复古暖调
inline const ThemeColors kGruvboxDark = {
    {"#1D2021"}, {"#282828"}, {"#3C3836"}, {"#17191A"},
    {"#EBDBB2"}, {"#A89984"}, {"#6E6259"},
    {"#504945"}, {"#3C3836"},
    {"#FE8019"}, {"#FE9E4A"}, {"#D65D0E"}, {"#504945"},
    {"#3C3836"}, {"#32302E"}, {"#4A4540"}, {"#4A4540"}, {"#5A544E"},
    {"#32302E"}, {"#FB4934"}, {"#5A2020"},
    {"#B8BB26"}, {"#FABD2F"}, {"#FB4934"},
    {"#FFF8E7"}, {"#FFF8E7"}, {"#1D2021"},
};

// 8. 阳化深色 Solarized Dark：ethanschoonover.com 科学配色
inline const ThemeColors kSolarizedDark = {
    {"#002B36"}, {"#073642"}, {"#0E4A58"}, {"#00212B"},
    {"#839496"}, {"#586E75"}, {"#48626B"},
    {"#0A4754"}, {"#0A3B47"},
    {"#268BD2"}, {"#4A9FD8"}, {"#1E6FA5"}, {"#073642"},
    {"#0E4A58"}, {"#0A3B47"}, {"#10505F"}, {"#10505F"}, {"#15606F"},
    {"#0A3B47"}, {"#DC322F"}, {"#5A1F1F"},
    {"#859900"}, {"#B58900"}, {"#DC322F"},
    {"#FDF6E3"}, {"#FDF6E3"}, {"#002B36"},
};

// 9. 东京夜 Tokyo Night：tokyonight.org 官方
inline const ThemeColors kTokyoNight = {
    {"#16161E"}, {"#1A1B26"}, {"#24283B"}, {"#11111A"},
    {"#A9B1D6"}, {"#565F89"}, {"#41486B"},
    {"#414868"}, {"#2E344A"},
    {"#7AA2F7"}, {"#9AB5F9"}, {"#5E8AE8"}, {"#283457"},
    {"#24283B"}, {"#1F2430"}, {"#2B3246"}, {"#2B3246"}, {"#37415C"},
    {"#1F2430"}, {"#F7768E"}, {"#5A2230"},
    {"#9ECE6A"}, {"#E0AF68"}, {"#F7768E"},
    {"#C0CAF5"}, {"#C0CAF5"}, {"#16161E"},
};

// ===== 浅色主题（4）=====

// 10. 浅色经典：Windows 浅灰（默认浅色）
inline const ThemeColors kLightClassic = {
    {"#F5F5F5"}, {"#FFFFFF"}, {"#EDEDED"}, {"#FFFFFF"},
    {"#1E1E1E"}, {"#616161"}, {"#9E9E9E"},
    {"#C8C8C8"}, {"#D6D6D6"},
    {"#0A84FF"}, {"#3399FF"}, {"#0066CC"}, {"#C8E4FF"},
    {"#E4E4E4"}, {"#F0F0F0"}, {"#E0E0E0"}, {"#DCDCDC"}, {"#B8B8B8"},
    {"#E8E8E8"}, {"#C62828"}, {"#FFDAD6"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#1E1E1E"}, {"#FFFFFF"},
};

// 11. 浅蓝：天空 / 科研蓝
inline const ThemeColors kLightBlue = {
    {"#EFF5FB"}, {"#F7FAFD"}, {"#E1EAF4"}, {"#FFFFFF"},
    {"#1B3A5C"}, {"#5A7A99"}, {"#9AAEBD"},
    {"#B8CCDE"}, {"#CBD9E8"},
    {"#1E88E5"}, {"#4AA0EA"}, {"#1565C0"}, {"#BFDCF5"},
    {"#DCE8F4"}, {"#E9F1FA"}, {"#D3E3F3"}, {"#CFE0F0"}, {"#9FC3E3"},
    {"#E2EDF8"}, {"#C62828"}, {"#FFDAD6"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#0D3B66"}, {"#FFFFFF"},
};

// 12. 质感浅色 Material Light：Material Design 官方色
inline const ThemeColors kMaterialLight = {
    {"#FAFAFA"}, {"#FFFFFF"}, {"#F0F0F0"}, {"#FFFFFF"},
    {"#212121"}, {"#757575"}, {"#BDBDBD"},
    {"#DDDDDD"}, {"#E8E8E8"},
    {"#6200EE"}, {"#8133F2"}, {"#4A00B4"}, {"#E8D6FB"},
    {"#E8E8E8"}, {"#F5F5F5"}, {"#E0E0E0"}, {"#DBDBDB"}, {"#BDBDBD"},
    {"#EDEDED"}, {"#C62828"}, {"#FFDAD6"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#4A148C"}, {"#FFFFFF"},
};

// 13. 阳化浅色 Solarized Light
inline const ThemeColors kSolarizedLight = {
    {"#FDF6E3"}, {"#F5EED9"}, {"#E8DFC8"}, {"#FFF9EC"},
    {"#586E75"}, {"#839496"}, {"#A8B0B0"},
    {"#C8C1A5"}, {"#D8D2B8"},
    {"#268BD2"}, {"#4AA0D9"}, {"#1E6FA5"}, {"#DCD5B8"},
    {"#E7DFC4"}, {"#EFE8D0"}, {"#DFD7BC"}, {"#D8D0B4"}, {"#B8B093"},
    {"#EFE8D0"}, {"#DC322F"}, {"#F5D8D8"},
    {"#859900"}, {"#B58900"}, {"#DC322F"},
    {"#FFFFFF"}, {"#37525E"}, {"#FFFFFF"},
};

// ===== 高对比主题（2）=====

// 14. 高对比黑：Windows HC Black 风格（弱化装饰、纯黑白黄）
inline const ThemeColors kHighContrastBlack = {
    {"#000000"}, {"#000000"}, {"#000000"}, {"#000000"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#808080"},
    {"#FFFFFF"}, {"#FFFFFF"},
    {"#FFFF00"}, {"#FFFF66"}, {"#CCCC00"}, {"#000080"},
    {"#000000"}, {"#1A1A1A"}, {"#333333"}, {"#333333"}, {"#666666"},
    {"#1A1A1A"}, {"#FF5555"}, {"#5A1F1F"},
    {"#00FF00"}, {"#FFFF00"}, {"#FF0000"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#000000"},
};

// 15. 高对比白：Windows HC White 风格
inline const ThemeColors kHighContrastWhite = {
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
    {"#000000"}, {"#000000"}, {"#767676"},
    {"#000000"}, {"#000000"},
    {"#0000FF"}, {"#4444FF"}, {"#0000CC"}, {"#000080"},
    {"#E5E5E5"}, {"#F2F2F2"}, {"#CCCCCC"}, {"#CCCCCC"}, {"#999999"},
    {"#E5E5E5"}, {"#CC0000"}, {"#FFCCCC"},
    {"#006600"}, {"#997A00"}, {"#CC0000"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
};

// 主题目录（顺序即菜单顺序：深色 → 浅色 → 高对比）
inline const ThemeDescriptor kThemes[] = {
    {"dark-classic",      "深色经典",        "深色",   kDarkClassic},
    {"dark-blue",         "深蓝",            "深色",   kDarkBlue},
    {"nord",              "北欧 Nord",       "深色",   kNord},
    {"one-dark",          "One Dark",        "深色",   kOneDark},
    {"dracula",           "德古拉 Dracula",  "深色",   kDracula},
    {"monokai",           "Monokai",         "深色",   kMonokai},
    {"gruvbox-dark",      "格鲁夫深色",      "深色",   kGruvboxDark},
    {"solarized-dark",    "阳化深色",        "深色",   kSolarizedDark},
    {"tokyo-night",       "东京夜",          "深色",   kTokyoNight},
    {"light-classic",     "浅色经典",        "浅色",   kLightClassic},
    {"light-blue",        "浅蓝",            "浅色",   kLightBlue},
    {"material-light",    "质感浅色",        "浅色",   kMaterialLight},
    {"solarized-light",   "阳化浅色",        "浅色",   kSolarizedLight},
    {"hc-black",          "高对比黑",        "高对比", kHighContrastBlack},
    {"hc-white",          "高对比白",        "高对比", kHighContrastWhite},
};
inline constexpr int kThemeCount = 15;
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

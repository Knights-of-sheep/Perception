// ===== Perception 主题目录：25 套主题色板（唯一语义色定义）=====
// 架构：QSS 模板（theme_template.qss）含 @token@ 占位符，
//       切换主题时 ThemeManager 用本文件色板逐 token 渲染，得到最终 QSS。
//       色值来源：经典配色（VS Code / Nord / Dracula / One Dark / Monokai /
//       Gruvbox / Solarized / Tokyo Night / Material / Rosé Pine / Catppuccin /
//       Everforest / Kanagawa / Night Owl / Ayu / GitHub Primer / Windows HC）
//       官方色板，色盲友好序沿用 Okabe-Ito。
// 注意：每套主题需通过 build/_theme_check.py 的 WCAG 对比度校验
//       （text/textWeak/textDisabled/textOnSelection/textOnAccent 与各背景层级）。
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
    // ---- 交互反馈 ----
    QColor dockDropHighlight;  // Dock 拖拽放置高亮（VSCode sash.activeBorder 风格：
                               // 深色主题用亮蓝系、浅色主题用深蓝系、高对比用亮青系）
};

struct ThemeDescriptor {
    const char* id;     // 持久化 id（QSettings）
    const char* name;   // 菜单显示名
    const char* family; // 类别：深色 / 浅色 / 高对比（菜单分组用）
    ThemeColors colors;
};

// ===== 深色主题（15）=====

// 1. 深色经典：产品默认（vsCode Dark+ 系）
inline const ThemeColors kDarkClassic = {
    {"#1E1E1E"}, {"#252526"}, {"#3C3C3C"}, {"#161616"},
    {"#D4D4D4"}, {"#9D9D9D"}, {"#808080"},
    {"#454545"}, {"#3F3F3F"},
    {"#0A84FF"}, {"#3399FF"}, {"#0066CC"}, {"#094771"},
    {"#3C3C3C"}, {"#2A2A2A"}, {"#333333"}, {"#4A4A4A"}, {"#5A5A5A"},
    {"#2D2D30"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#4EC9B0"}, {"#CCA700"}, {"#F14C4C"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#001020"},
    {"#4FC1FF"},
};

// 2. 深蓝：冷调蓝灰（海洋 / 数据监视风格）
inline const ThemeColors kDarkBlue = {
    {"#15202B"}, {"#1B2838"}, {"#2B3A4B"}, {"#101822"},
    {"#D8E1EA"}, {"#93A3B4"}, {"#75818F"},
    {"#3A4A5D"}, {"#334252"},
    {"#2E8FFF"}, {"#5AA8FF"}, {"#1A6FD0"}, {"#123E6B"},
    {"#2B3A4B"}, {"#202E3D"}, {"#32445A"}, {"#35465A"}, {"#40556C"},
    {"#223243"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#4EC9B0"}, {"#CCA700"}, {"#F14C4C"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#0A1622"},
    {"#5AA8FF"},
};

// 3. 北欧 Nord：官方北极蓝（nordtheme.com，frost/aurora）
inline const ThemeColors kNord = {
    {"#2E3440"}, {"#3B4252"}, {"#434C5E"}, {"#272C36"},
    {"#D8DEE9"}, {"#98A2B3"}, {"#8A96A8"},
    {"#4C566A"}, {"#434C5E"},
    {"#88C0D0"}, {"#A3D3DF"}, {"#5E81AC"}, {"#434C5E"},
    {"#434C5E"}, {"#3B4252"}, {"#4C566A"}, {"#4C566A"}, {"#5E81AC"},
    {"#3B4252"}, {"#D87F88"}, {"#5A2E33"},
    {"#A3BE8C"}, {"#EBCB8B"}, {"#BF616A"},
    {"#ECEFF4"}, {"#ECEFF4"}, {"#2E3440"},
    {"#A3D3DF"},
};

// 4. One Dark：Atom / VS Code 默认深色
inline const ThemeColors kOneDark = {
    {"#21252B"}, {"#282C34"}, {"#323842"}, {"#1B1E24"},
    {"#ABB2BF"}, {"#7F8899"}, {"#757E8E"},
    {"#3A4048"}, {"#31363E"},
    {"#61AFEF"}, {"#82C2F2"}, {"#3E8FD9"}, {"#1B3852"},
    {"#323842"}, {"#2C313A"}, {"#3A4048"}, {"#3A4048"}, {"#4B5263"},
    {"#2C313A"}, {"#E06C75"}, {"#5A2A2F"},
    {"#98C379"}, {"#E5C07B"}, {"#E06C75"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#21252B"},
    {"#82C2F2"},
};

// 5. 德古拉 Dracula：draculatheme.com 官方
inline const ThemeColors kDracula = {
    {"#21222C"}, {"#282A36"}, {"#343746"}, {"#191A24"},
    {"#F8F8F2"}, {"#9096A6"}, {"#767D90"},
    {"#44475A"}, {"#3A3D4D"},
    {"#BD93F9"}, {"#CEAEFB"}, {"#9A6FE3"}, {"#44475A"},
    {"#343746"}, {"#2F3140"}, {"#3E4152"}, {"#3E4152"}, {"#4A4E60"},
    {"#2F3140"}, {"#FF8A8A"}, {"#5A1F1F"},
    {"#50FA7B"}, {"#F1FA8C"}, {"#FF5555"},
    {"#FFFFFF"}, {"#F8F8F2"}, {"#1E1E2E"},
    {"#CEAEFB"},
};

// 6. Monokai：Sublime Text 经典
inline const ThemeColors kMonokai = {
    {"#1F201C"}, {"#272822"}, {"#33352E"}, {"#181913"},
    {"#F8F8F2"}, {"#A0A096"}, {"#7D7E74"},
    {"#3E4038"}, {"#33352E"},
    {"#A6E22E"}, {"#BCE55C"}, {"#85B41F"}, {"#49483E"},
    {"#33352E"}, {"#2C2E27"}, {"#3E4038"}, {"#3E4038"}, {"#4A4C42"},
    {"#2C2E27"}, {"#F92672"}, {"#5A1F2E"},
    {"#A6E22E"}, {"#E6DB74"}, {"#F92672"},
    {"#FFFFFF"}, {"#F8F8F2"}, {"#1E1E1E"},
    {"#BCE55C"},
};

// 7. 格鲁夫深色 Gruvbox：morhetz 复古暖调
inline const ThemeColors kGruvboxDark = {
    {"#1D2021"}, {"#282828"}, {"#3C3836"}, {"#17191A"},
    {"#EBDBB2"}, {"#A89984"}, {"#867A6E"},
    {"#504945"}, {"#3C3836"},
    {"#FE8019"}, {"#FE9E4A"}, {"#D65D0E"}, {"#504945"},
    {"#3C3836"}, {"#32302E"}, {"#4A4540"}, {"#4A4540"}, {"#5A544E"},
    {"#32302E"}, {"#FB4934"}, {"#5A2020"},
    {"#B8BB26"}, {"#FABD2F"}, {"#FB4934"},
    {"#FFF8E7"}, {"#FFF8E7"}, {"#1D2021"},
    {"#FE9E4A"},
};

// 8. 阳化深色 Solarized Dark：ethanschoonover.com 科学配色
inline const ThemeColors kSolarizedDark = {
    {"#002B36"}, {"#073642"}, {"#0B3B48"}, {"#00212B"},
    {"#93A1A1"}, {"#839496"}, {"#6D8282"},
    {"#0A4754"}, {"#0A3B47"},
    {"#268BD2"}, {"#4A9FD8"}, {"#1E6FA5"}, {"#073642"},
    {"#0E4A58"}, {"#0A3B47"}, {"#10505F"}, {"#10505F"}, {"#15606F"},
    {"#0A3B47"}, {"#E05B57"}, {"#5A1F1F"},
    {"#859900"}, {"#B58900"}, {"#DC322F"},
    {"#FDF6E3"}, {"#FDF6E3"}, {"#00212B"},
    {"#4A9FD8"},
};

// 9. 东京夜 Tokyo Night：tokyonight.org 官方
inline const ThemeColors kTokyoNight = {
    {"#16161E"}, {"#1A1B26"}, {"#24283B"}, {"#11111A"},
    {"#A9B1D6"}, {"#7E8AAD"}, {"#6A7390"},
    {"#414868"}, {"#2E344A"},
    {"#7AA2F7"}, {"#9AB5F9"}, {"#5E8AE8"}, {"#283457"},
    {"#24283B"}, {"#1F2430"}, {"#2B3246"}, {"#2B3246"}, {"#37415C"},
    {"#1F2430"}, {"#F7768E"}, {"#5A2230"},
    {"#9ECE6A"}, {"#E0AF68"}, {"#F7768E"},
    {"#C0CAF5"}, {"#C0CAF5"}, {"#16161E"},
    {"#9AB5F9"},
};

// ===== 浅色主题（7）=====

// 10. 浅色经典：Windows 浅灰（默认浅色）
inline const ThemeColors kLightClassic = {
    {"#F5F5F5"}, {"#FFFFFF"}, {"#EDEDED"}, {"#FFFFFF"},
    {"#1E1E1E"}, {"#616161"}, {"#8A8A8A"},
    {"#C8C8C8"}, {"#D6D6D6"},
    {"#0A84FF"}, {"#3399FF"}, {"#0066CC"}, {"#C8E4FF"},
    {"#E4E4E4"}, {"#F0F0F0"}, {"#E0E0E0"}, {"#DCDCDC"}, {"#B8B8B8"},
    {"#E8E8E8"}, {"#C62828"}, {"#C62828"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#1E1E1E"}, {"#001E38"},
    {"#007ACC"},
};

// 11. 浅蓝：天空 / 科研蓝
inline const ThemeColors kLightBlue = {
    {"#EFF5FB"}, {"#F7FAFD"}, {"#E1EAF4"}, {"#FFFFFF"},
    {"#1B3A5C"}, {"#5A7A99"}, {"#748DA0"},
    {"#B8CCDE"}, {"#CBD9E8"},
    {"#1E88E5"}, {"#4AA0EA"}, {"#1565C0"}, {"#BFDCF5"},
    {"#DCE8F4"}, {"#E9F1FA"}, {"#D3E3F3"}, {"#CFE0F0"}, {"#9FC3E3"},
    {"#E2EDF8"}, {"#C62828"}, {"#C62828"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#0D3B66"}, {"#0A1622"},
    {"#1565C0"},
};

// 12. 质感浅色 Material Light：Material Design 官方色
inline const ThemeColors kMaterialLight = {
    {"#FAFAFA"}, {"#FFFFFF"}, {"#F0F0F0"}, {"#FFFFFF"},
    {"#212121"}, {"#757575"}, {"#969696"},
    {"#DDDDDD"}, {"#E8E8E8"},
    {"#6200EE"}, {"#8133F2"}, {"#4A00B4"}, {"#E8D6FB"},
    {"#E8E8E8"}, {"#F5F5F5"}, {"#E0E0E0"}, {"#DBDBDB"}, {"#BDBDBD"},
    {"#EDEDED"}, {"#C62828"}, {"#C62828"},
    {"#2E7D32"}, {"#B58900"}, {"#C62828"},
    {"#FFFFFF"}, {"#4A148C"}, {"#FFFFFF"},
    {"#5B3DF5"},
};

// 13. 阳化浅色 Solarized Light
inline const ThemeColors kSolarizedLight = {
    {"#FDF6E3"}, {"#F5EED9"}, {"#E8DFC8"}, {"#FFF9EC"},
    {"#45606A"}, {"#657B83"}, {"#7A8B8B"},
    {"#C8C1A5"}, {"#D8D2B8"},
    {"#268BD2"}, {"#4AA0D9"}, {"#1E6FA5"}, {"#DCD5B8"},
    {"#E7DFC4"}, {"#EFE8D0"}, {"#DFD7BC"}, {"#D8D0B4"}, {"#B8B093"},
    {"#EFE8D0"}, {"#DC322F"}, {"#DC322F"},
    {"#859900"}, {"#B58900"}, {"#DC322F"},
    {"#FFFFFF"}, {"#37525E"}, {"#00212B"},
    {"#1E6FA5"},
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
    {"#6FC3DF"},
};

// 15. 高对比白：Windows HC White 风格
inline const ThemeColors kHighContrastWhite = {
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
    {"#000000"}, {"#767676"}, {"#767676"},
    {"#000000"}, {"#000000"},
    {"#0000FF"}, {"#4444FF"}, {"#0000CC"}, {"#000080"},
    {"#E5E5E5"}, {"#F2F2F2"}, {"#CCCCCC"}, {"#CCCCCC"}, {"#999999"},
    {"#E5E5E5"}, {"#CC0000"}, {"#CC0000"},
    {"#006600"}, {"#997A00"}, {"#CC0000"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#FFFFFF"},
    {"#0063B1"},
};

// ===== 深色主题（续 16-21）=====

// 16. Rosé Pine：柔和玫瑰紫（rosepinetheme.com 官方）
inline const ThemeColors kRosePine = {
    {"#191724"}, {"#1F1D2E"}, {"#26233A"}, {"#13101D"},
    {"#E0DEF4"}, {"#908CAA"}, {"#6E6A86"},
    {"#403D52"}, {"#2A273F"},
    {"#EBBCBA"}, {"#F2D2CF"}, {"#C4A0A2"}, {"#403D52"},
    {"#26233A"}, {"#2A273F"}, {"#26233A"}, {"#312F49"}, {"#524F67"},
    {"#1F1D2E"}, {"#EB6F92"}, {"#5C2440"},
    {"#9CCFD8"}, {"#F6C177"}, {"#EB6F92"},
    {"#FFFFFF"}, {"#E0DEF4"}, {"#191724"},
    {"#F2D2CF"},
};

// 17. Catppuccin Mocha：奶油拿铁紫（catppuccin.com 官方）
inline const ThemeColors kCatppuccinMocha = {
    {"#1E1E2E"}, {"#181825"}, {"#313244"}, {"#11111B"},
    {"#CDD6F4"}, {"#A6ADC8"}, {"#6C7086"},
    {"#45475A"}, {"#313244"},
    {"#89B4FA"}, {"#9CC4FB"}, {"#6F9CF5"}, {"#45475A"},
    {"#313244"}, {"#363A4F"}, {"#313244"}, {"#3C4056"}, {"#585B70"},
    {"#181825"}, {"#F38BA8"}, {"#6B2B3C"},
    {"#A6E3A1"}, {"#F9E2AF"}, {"#F38BA8"},
    {"#FFFFFF"}, {"#CDD6F4"}, {"#11111B"},
    {"#9CC4FB"},
};

// 18. 常绿深色 Everforest：柔和绿调（原始森林风）
inline const ThemeColors kEverforestDark = {
    {"#2D353B"}, {"#272E34"}, {"#343F44"}, {"#232A2F"},
    {"#D3C6AA"}, {"#9DA9A0"}, {"#7F897D"},
    {"#4B565C"}, {"#3D484D"},
    {"#7FBBB3"}, {"#9BC9C2"}, {"#689A93"}, {"#4B565C"},
    {"#343F44"}, {"#3D484D"}, {"#343F44"}, {"#404C52"}, {"#5C6A66"},
    {"#272E34"}, {"#E67E80"}, {"#5A3335"},
    {"#A7C080"}, {"#DBBC7F"}, {"#E67E80"},
    {"#FFFFFF"}, {"#DED2B4"}, {"#232A2F"},
    {"#9BC9C2"},
};

// 19. 神奈川 Kanagawa：和风水墨（rebelot.github.io 官方）
inline const ThemeColors kKanagawa = {
    {"#1F1F28"}, {"#16161D"}, {"#2A2A37"}, {"#181820"},
    {"#DCD7BA"}, {"#938AA9"}, {"#717C7C"},
    {"#363646"}, {"#2A2A37"},
    {"#7E9CD8"}, {"#9DB4E6"}, {"#6483BD"}, {"#363646"},
    {"#2A2A37"}, {"#2D2D3D"}, {"#2A2A37"}, {"#33334A"}, {"#54546D"},
    {"#16161D"}, {"#E46876"}, {"#5C2A35"},
    {"#98BB6C"}, {"#E6C384"}, {"#E46876"},
    {"#FFFFFF"}, {"#DCD7BA"}, {"#16161D"},
    {"#9DB4E6"},
};

// 20. 夜猫头鹰 Night Owl：深蓝星空（sdras/night-owl 官方）
inline const ThemeColors kNightOwl = {
    {"#011627"}, {"#0A1E2E"}, {"#1A2C3E"}, {"#010E1A"},
    {"#D6DEEB"}, {"#99A6BD"}, {"#5F6E85"},
    {"#33415C"}, {"#243349"},
    {"#82AAFF"}, {"#9CC2FF"}, {"#5F8DE0"}, {"#33415C"},
    {"#1A2C3E"}, {"#223449"}, {"#1A2C3E"}, {"#253A50"}, {"#46597A"},
    {"#0A1E2E"}, {"#FF5874"}, {"#5A2A3A"},
    {"#ADDB67"}, {"#F78C6C"}, {"#FF5874"},
    {"#FFFFFF"}, {"#D6DEEB"}, {"#011627"},
    {"#9CC2FF"},
};

// 21. Ayu Dark：日式黄昏（teabyii/ayu 官方）
inline const ThemeColors kAyuDark = {
    {"#0B0E14"}, {"#131722"}, {"#1A1F2B"}, {"#0D1017"},
    {"#B3B1AD"}, {"#8A9199"}, {"#606670"},
    {"#232A36"}, {"#1A2029"},
    {"#E6B450"}, {"#F0C06A"}, {"#C89A3E"}, {"#232A36"},
    {"#1A1F2B"}, {"#20262F"}, {"#1A1F2B"}, {"#242B36"}, {"#3C4655"},
    {"#131722"}, {"#F07178"}, {"#5C2A30"},
    {"#AAD94C"}, {"#FF8F40"}, {"#F07178"},
    {"#FFFFFF"}, {"#B3B1AD"}, {"#0B0E14"},
    {"#F0C06A"},
};

// ===== 浅色主题（续 22-24）=====

// 22. Rosé Pine Dawn：玫瑰晨光（rose-pine 浅色版）
inline const ThemeColors kRosePineDawn = {
    {"#FAF4ED"}, {"#FFFAF3"}, {"#F2E9DE"}, {"#FFFFFF"},
    {"#575279"}, {"#797593"}, {"#8A8598"},
    {"#D7D0C1"}, {"#E3DDCE"},
    {"#56949F"}, {"#7EB0B8"}, {"#427A85"}, {"#E3DCD0"},
    {"#F2E9DE"}, {"#E7DED0"}, {"#F2E9DE"}, {"#E8DFD1"}, {"#C6BEB0"},
    {"#FFFAF3"}, {"#B4637A"}, {"#B4637A"},
    {"#286983"}, {"#EA9D34"}, {"#B4637A"},
    {"#FFFFFF"}, {"#575279"}, {"#072023"},
    {"#56949F"},
};

// 23. Catppuccin Latte：拿铁咖啡（catppuccin 浅色版）
inline const ThemeColors kCatppuccinLatte = {
    {"#EFF1F5"}, {"#E6E9EF"}, {"#DCE0E8"}, {"#FFFFFF"},
    {"#4C4F69"}, {"#6C6F85"}, {"#85889A"},
    {"#CCD0DA"}, {"#DCE0E8"},
    {"#1E66F5"}, {"#4D86F7"}, {"#1548C9"}, {"#CCD0DA"},
    {"#DCE0E8"}, {"#D3D7E2"}, {"#DCE0E8"}, {"#D0D4DE"}, {"#ACB0BE"},
    {"#E6E9EF"}, {"#D20F39"}, {"#D20F39"},
    {"#40A02B"}, {"#DF8E1D"}, {"#D20F39"},
    {"#FFFFFF"}, {"#4C4F69"}, {"#FFFFFF"},
    {"#1E66F5"},
};

// 24. GitHub 浅色：Primer 官方（github.com/primer 色板）
inline const ThemeColors kGithubLight = {
    {"#FFFFFF"}, {"#F6F8FA"}, {"#EAEEF2"}, {"#FFFFFF"},
    {"#1F2328"}, {"#59636E"}, {"#8C959F"},
    {"#D1D9E0"}, {"#E5E9EE"},
    {"#0969DA"}, {"#2188FF"}, {"#0A55B0"}, {"#D1D9E0"},
    {"#EAEEF2"}, {"#EEF1F4"}, {"#EAEEF2"}, {"#D9DEE3"}, {"#A0A8B3"},
    {"#F6F8FA"}, {"#CF222E"}, {"#CF222E"},
    {"#1A7F37"}, {"#9A6700"}, {"#CF222E"},
    {"#FFFFFF"}, {"#1F2328"}, {"#FFFFFF"},
    {"#0969DA"},
};

// ===== 高对比主题（续 25）=====

// 25. 高对比蓝：Windows HC Blue 风格
inline const ThemeColors kHighContrastBlue = {
    {"#003366"}, {"#003366"}, {"#00407A"}, {"#002244"},
    {"#FFFFFF"}, {"#CCE5FF"}, {"#AACCEE"},
    {"#FFFFFF"}, {"#99CCFF"},
    {"#FFFF00"}, {"#FFFF66"}, {"#CCCC00"}, {"#000080"},
    {"#00407A"}, {"#005599"}, {"#00407A"}, {"#0066AA"}, {"#3399FF"},
    {"#003366"}, {"#FF9999"}, {"#660000"},
    {"#00FF00"}, {"#FFFF00"}, {"#FF0000"},
    {"#FFFFFF"}, {"#FFFFFF"}, {"#000000"},
    {"#6FC3DF"},
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
    {"rose-pine",         "Rosé Pine",       "深色",   kRosePine},
    {"catppuccin-mocha",  "Catppuccin",      "深色",   kCatppuccinMocha},
    {"everforest-dark",   "常绿深色",        "深色",   kEverforestDark},
    {"kanagawa",          "神奈川",          "深色",   kKanagawa},
    {"night-owl",         "夜猫头鹰",        "深色",   kNightOwl},
    {"ayu-dark",          "Ayu 深色",        "深色",   kAyuDark},
    {"light-classic",     "浅色经典",        "浅色",   kLightClassic},
    {"light-blue",        "浅蓝",            "浅色",   kLightBlue},
    {"material-light",    "质感浅色",        "浅色",   kMaterialLight},
    {"solarized-light",   "阳化浅色",        "浅色",   kSolarizedLight},
    {"rose-pine-dawn",    "Rosé Pine 晨光",  "浅色",   kRosePineDawn},
    {"catppuccin-latte",  "Catppuccin 拿铁", "浅色",   kCatppuccinLatte},
    {"github-light",      "GitHub 浅色",     "浅色",   kGithubLight},
    {"hc-black",          "高对比黑",        "高对比", kHighContrastBlack},
    {"hc-white",          "高对比白",        "高对比", kHighContrastWhite},
    {"hc-blue",           "高对比蓝",        "高对比", kHighContrastBlue},
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

// ===== 深色主题色板（15 套）=====
// 006-constitution-refactor：自 theme_catalog.h 拆分（色板按明暗/对比分组）。
// 色值来源：经典配色（VS Code / Nord / Dracula / One Dark / Monokai / Gruvbox /
// Solarized / Tokyo Night / Rosé Pine / Catppuccin / Everforest / Kanagawa /
// Night Owl / Ayu）官方色板，色盲友好序沿用 Okabe-Ito。
#pragma once

#include "theme_types.h"

namespace perception {
namespace ui {
namespace theme {

// 1. 深色经典：产品默认（vsCode Dark+ 系）
// 注：border / borderWeak 提亮以满足「面板/子窗口分隔线必须清晰可见」的设计反馈；
//     仍属深色 UI 范畴（≥#4A4A4A），与 VS Code 默认 Dark+ (#3F3F3F) 相比略亮，
//     实测在 1280px 截图中 ≥1px 即可辨识。1px 边若仍嫌纤细可加大 subwindowView border
//     至 2px（在 viewBg 容器上 ΔL≈80，可达 2x 对比度）。
inline const ThemeColors kDarkClassic = {
    {"#1E1E1E"}, {"#252526"}, {"#3C3C3C"}, {"#161616"},
    {"#D4D4D4"}, {"#9D9D9D"}, {"#808080"},
    {"#5A5A5A"}, {"#4A4A4A"},
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

}  // namespace theme
}  // namespace ui
}  // namespace perception

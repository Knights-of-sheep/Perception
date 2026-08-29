// ===== 浅色主题色板（10 套）=====
// 006-constitution-refactor：自 theme_catalog.h 拆分（色板按明暗/对比分组）。
// 色值来源：经典配色（Solarized / Rosé Pine / Catppuccin / GitHub Primer 等）官方色板。
#pragma once

#include "theme_types.h"

namespace perception {
namespace ui {
namespace theme {

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

}  // namespace theme
}  // namespace ui
}  // namespace perception

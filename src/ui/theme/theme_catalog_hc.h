// ===== 高对比主题色板（3 套）=====
// 006-constitution-refactor：自 theme_catalog.h 拆分（色板按明暗/对比分组）。
// 色值来源：Windows 高对比模式官方配色（弱化装饰、纯黑白黄蓝）。
#pragma once

#include "theme_types.h"

namespace perception {
namespace ui {
namespace theme {

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

}  // namespace theme
}  // namespace ui
}  // namespace perception

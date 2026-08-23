// ===== Perception 主题色板（唯一语义色定义）=====
// 规范：docs/design/ui-guidelines.md §3 / §4.5（Token 单点定义，QSS/C++/VTK 三方共用）
#pragma once

#include <QColor>

namespace perception {
namespace ui {
namespace theme {

// ===== 分层背景 =====
inline const QColor kWindowBg  {"#1E1E1E"};  // BG_WINDOW   主窗口背景
inline const QColor kPanelBg   {"#252526"};  // BG_PANEL    Dock / 侧边栏 / 图例底
inline const QColor kControlBg {"#3C3C3C"};  // BG_CONTROL   按钮、输入框、树选中底
inline const QColor kViewBg    {"#161616"};  // BG_VIEW      中央视图底色（绘图区）

// ===== 前景与边框 =====
inline const QColor kText          {"#D4D4D4"};  // FG_TEXT          主文字
inline const QColor kTextWeak      {"#9D9D9D"};  // FG_TEXT_WEAK     次要文字
inline const QColor kTextDisabled  {"#6E6E6E"};  // FG_TEXT_DISABLED 禁用文字
inline const QColor kBorder        {"#454545"};  // BORDER           边框、分隔线
inline const QColor kBorderWeak    {"#3F3F3F"};  // BORDER_WEAK      弱分隔线

// ===== 强调与状态 =====
inline const QColor kAccent      {"#0A84FF"};  // ACCENT      全 UI 唯一主色
inline const QColor kSelectionBg {"#094771"};  // SELECTION_BG 列表/树选中底
inline const QColor kSuccess     {"#4EC9B0"};  // SUCCESS
inline const QColor kWarning     {"#CCA700"};  // WARNING
inline const QColor kDanger      {"#F14C4C"};  // DANGER

// ===== 曲线序列色板（Okabe-Ito，色盲友好，ui-guidelines §3.2）=====
inline const QColor kCurvePalette[] = {
    QColor{"#0072B2"},  // 蓝
    QColor{"#E69F00"},  // 橙
    QColor{"#009E73"},  // 绿
    QColor{"#D55E00"},  // 红
    QColor{"#56B4E9"},  // 天蓝
    QColor{"#CC79A7"},  // 紫
    QColor{"#F0E442"},  // 黄
    QColor{"#000000"},  // 黑
};
inline constexpr int kCurvePaletteSize = 8;

}  // namespace theme
}  // namespace ui
}  // namespace perception

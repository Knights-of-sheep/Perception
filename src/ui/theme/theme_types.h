// ===== 主题语义 token 结构（唯一定义）=====
// ThemeColors：与 theme_template.qss 占位符一一对应的语义色（小驼峰，@name@ 形式）。
// ThemeDescriptor：单套主题的元数据（id 持久化 / 菜单名 / family 分组）。
// 006-constitution-refactor：结构定义自 theme_catalog.h 提取为独立头
//（theme_catalog.h 与 theme_catalog_{dark,light,hc}.h 共用，避免循环依赖）。
#pragma once

#include <QColor>

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
    // ---- 弹窗层（008-unify-dialog-styling）----
    QColor dialogBg;  // BG_DIALOG 弹窗背景层（默认无效色 = 渲染时由 deriveDialogBg 派生；
                      // 显式赋值优先，供未来定制钩子）
};

struct ThemeDescriptor {
    const char* id;     // 持久化 id（QSettings）
    const char* name;   // 菜单显示名
    const char* family; // 类别：深色 / 浅色 / 高对比（菜单分组用）
    ThemeColors colors;
};

}  // namespace theme
}  // namespace ui
}  // namespace perception

// ===== QFileDialog 主题化 QSS 生成（008-unify-dialog-styling 跟进）=====
// 问题：Qt 5.15 的 QFileDialog（DontUseNativeDialog）内部 QFileWidget 在构造中
//       setStyleSheet 设置自身硬编码样式表，会屏蔽应用级 QSS——导致内嵌文件对话框
//       的侧边栏 / Look-in 行 / 底部按钮 / 视图等区域停留在浅色原生观感，与其他
//       弹窗（统一 dialogBg + titleBarRow）视觉不一致。
// 方案：由本函数按当前主题色板生成完整 QSS，ThemedFileDialog 构造时
//       fileDialog_->setStyleSheet(...) 显式注入覆盖（QFileDialog 的样式表
//       级联作用于其所有子部件，含 QFileWidget）。
// 一致性：背景 dialogBg 用与全局 QSS 相同的派生路径（deriveDialogBg），
//         控件色全部取自 ThemeColors 同一语义 token（ui-guidelines §3/§4.1）。
#pragma once

#include <QColor>
#include <QString>

namespace perception {
namespace ui {
namespace theme {
struct ThemeColors;
}  // namespace theme

// 生成内嵌 QFileDialog 的主题化样式表（dialogBg 由调用方按派生规则传入）
QString buildFileDialogQss(const theme::ThemeColors& c, const QColor& dialogBg);

}  // namespace ui
}  // namespace perception

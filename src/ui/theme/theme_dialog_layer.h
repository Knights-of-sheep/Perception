// ===== 弹窗背景层派生（008-unify-dialog-styling）=====
// 弹窗背景层语义色 dialogBg 的派生规则（research R1 修订 / data-model §2）：
//   - Dark family：windowBg 的 HSL lightness +8 个百分点，clamp 上限 70%
//   - Light family：windowBg 的 HSL lightness −8 个百分点，clamp 下限 30%
//   - High Contrast family：不派生（返回无效色，渲染时回退 windowBg，
//                           层次靠 QSS 1px 高对比边框达成）
//   - 对比度保护（FR-003 可读性硬约束）：提亮/压暗后若 dialogBg 与正文 text 的
//     WCAG 对比度低于 4.5:1（如 Solarized 类低对比色板），自动逐级回调派生量，
//     至对比度达标为止；保护降级后层次差异可能小于 8pt，但始终保有可感知差异。
// 保持色相与饱和度不变 → 与主界面同一配色家族（FR-003）；
// 单一实现点自动覆盖全部 25 套主题，避免逐套人工取值导致层次不一致（SC-001）。
// 注：family 取值来自 theme_catalog.h 的 ThemeDescriptor::family
//     （"Dark" / "Light" / "High Contrast"）。
#pragma once

#include <QColor>

namespace perception {
namespace ui {
namespace theme {

// 派生弹窗背景层颜色；输入无效色或非 Dark/Light family 时返回无效色，
// 由调用方（ThemeManager::renderQss）回退使用 windowBg。
QColor deriveDialogBg(const QColor& windowBg, const QColor& text, const char* family);

}  // namespace theme
}  // namespace ui
}  // namespace perception

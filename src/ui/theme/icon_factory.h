// ===== IconFactory：动作图标五态构造（T-04/T-06 运行时派生）=====
// 对应 contracts/icon-action-map.md §5：disabled/selected 由本工厂运行时派生
// （research 决策 3）。普通态不再使用原始 PNG 灰阶：原图为固定灰阶
// （192/96/32 三层），深色主题下暗部不可见、浅色主题下亮部不可见，
// 因此普通态染成主题弱文字色（textWeak）、选中/checked 态染成选中文字色
// （textOnSelection），随主题热切换且对比度经 WCAG 校验（≥3:1）。
#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

namespace perception {
namespace ui {
namespace theme {

// 从 qrc 原始 PNG 构造带多状态的 QIcon。
// 原图为灰色单色图标，故用 SourceIn 按 alpha 重染即可。
// 派生规则：
//   - Normal / Off：原图染 FG_TEXT_WEAK（随主题，深浅主题均清晰）
//   - Disabled：原图染 FG_TEXT_DISABLED + 40% 透明度（T-04）
//   - Selected / checked(On)：原图染 FG_TEXT_ON_SELECTION。checked 按钮容器
//     背景为 selectionBg（QSS `:checked { background: @selectionBg@; }`），
//     图标须用选中态文字色——若沿用 textWeak，在高对比主题（如 hc-white
//     黑底）下对比仅 ~1.3:1 不可见（曾引入该回归，已修复）。
class IconFactory {
public:
    // 构造动作图标；颜色取自 ThemeManager::current()->colors，由调用方传入（保持纯函数）
    static QIcon actionIcon(const QString& iconId,
                            const QColor& textWeak,
                            const QColor& textOnSelection,
                            const QColor& textDisabled,
                            const QColor& accent);

private:
    static QPixmap derive(const QPixmap& src, const QColor& color, int alpha);
};

}  // namespace theme
}  // namespace ui
}  // namespace perception

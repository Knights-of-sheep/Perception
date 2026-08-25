// ===== IconFactory：动作图标五态构造（T-04/T-06 运行时派生）=====
// 对应 contracts/icon-action-map.md §5：normal/hover/pressed 由 QSS 容器 + 原始 PNG
// 呈现，disabled/selected 由本工厂运行时派生（research 决策 3）。
#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

namespace perception {
namespace ui {
namespace theme {

// 从 qrc 原始 PNG 构造带多状态的 QIcon。
// 原图为 FG_TEXT 单色实心，故用 SourceIn 按 alpha 重染即可。
// 派生规则：
//   - Disabled：原图染 FG_TEXT_DISABLED + 40% 透明度（T-04）
//   - Selected / checked(On)：原图染 ACCENT（T-06，双通道可辨）
class IconFactory {
public:
    // 构造动作图标；颜色取自 ThemeManager::current()->colors，由调用方传入（保持纯函数）
    static QIcon actionIcon(const QString& iconId,
                            const QColor& textDisabled,
                            const QColor& accent);

private:
    static QPixmap derive(const QPixmap& src, const QColor& color, int alpha);
};

}  // namespace theme
}  // namespace ui
}  // namespace perception

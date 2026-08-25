#include "ui/theme/icon_factory.h"

#include <QPainter>
#include <QtGlobal>

namespace perception {
namespace ui {
namespace theme {

QIcon IconFactory::actionIcon(const QString& iconId,
                              const QColor& textWeak,
                              const QColor& textOnSelection,
                              const QColor& textDisabled,
                              const QColor& accent) {
    QIcon icon;
    static const int kSizes[] = {16, 24, 32};
    for (int s : kSizes) {
        const QString path =
            QStringLiteral(":/perception/icons/icons/png/actions/%1-%2.png").arg(iconId).arg(s);
        QPixmap base(path);
        if (base.isNull()) {
            // FR-008：图标资源缺失 → 记录错误日志 + QIcon 回退到已注册档位，程序不崩溃
            qWarning("IconFactory: missing resource %s (iconId=%s, size=%d)",
                     qUtf8Printable(path), qUtf8Printable(iconId), s);
            continue;
        }
        // 普通态染成主题弱文字色：原始 PNG 为固定灰阶（192/96/32），
        // 深色主题暗部不可见、浅色主题亮部不可见；textWeak 随主题热切换且对比度
        // 已由 _theme_check.py 校验（≥3:1 on 各背景），保证所有主题下图标清晰。
        const QPixmap themed = derive(base, textWeak, 255);
        // 选中/checked 态：容器背景为 selectionBg，图标须用 textOnSelection
        // （高对比主题下 textWeak 在 selectionBg 上对比仅 ~1.3:1，曾回归）。
        const QPixmap selected = derive(base, textOnSelection, 255);
        icon.addPixmap(themed);                                  // Normal / Off
        icon.addPixmap(derive(base, textDisabled, 102), QIcon::Disabled, QIcon::Off);  // T-04：40% 透明
        // T-06：选中态由容器背景 selectionBg 表达；图标用 textOnSelection，
        // 与 selectionBg 的对比经校验（textOnSelection on selectionBg ≥4.5:1）。
        icon.addPixmap(selected, QIcon::Selected, QIcon::On);                            // 选中
        icon.addPixmap(selected, QIcon::Normal, QIcon::On);                              // checkable 选中
    }
    return icon;
}

QPixmap IconFactory::derive(const QPixmap& src, const QColor& color, int alpha) {
    QPixmap out(src.size());
    out.fill(Qt::transparent);
    {
        QPainter p(&out);
        p.drawPixmap(0, 0, src);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        QColor c = color;
        c.setAlpha(alpha);
        p.fillRect(out.rect(), c);
    }
    return out;
}

}  // namespace theme
}  // namespace ui
}  // namespace perception

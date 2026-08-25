#include "ui/theme/icon_factory.h"

#include <QPainter>
#include <QtGlobal>

namespace perception {
namespace ui {
namespace theme {

QIcon IconFactory::actionIcon(const QString& iconId,
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
        icon.addPixmap(base);                                  // Normal / Off（原始 PNG）
        icon.addPixmap(derive(base, textDisabled, 102), QIcon::Disabled, QIcon::Off);  // T-04：40% 透明
        icon.addPixmap(derive(base, accent, 255), QIcon::Selected, QIcon::On);         // T-06：ACCENT
        icon.addPixmap(derive(base, accent, 255), QIcon::Normal, QIcon::On);           // checkable 选中
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

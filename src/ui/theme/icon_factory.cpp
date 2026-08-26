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
        // 普通态染主题弱文字色（原图固定灰阶 192/96/32，深浅主题均不可直接复用）；
        // 选中/checked 态染选中文字色（容器背景 selectionBg，textWeak 在其上仅 ~1.3:1，曾回归）。
        const QPixmap themed = derive(base, textWeak, 255);
        const QPixmap selected = derive(base, textOnSelection, 255);
        icon.addPixmap(themed);                                                          // Normal / Off
        icon.addPixmap(derive(base, textDisabled, 102), QIcon::Disabled, QIcon::Off);    // T-04：40% 透明
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

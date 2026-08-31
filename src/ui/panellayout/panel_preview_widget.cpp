// ===== 面板布局实时示意图实现（010-panel-layout-settings FR-006）=====
#include "ui/panellayout/panel_preview_widget.h"

#include <QPainter>
#include <QRect>
#include <QString>

#include "ui/theme/theme_manager.h"
#include "ui/theme/theme_types.h"

namespace perception {
namespace ui {

namespace {
constexpr int kPreviewMargin = 10;  // 示意图内容区四边留白（px）
constexpr double kSideWidthRatio = 0.28;   // 左右面板宽度占可用宽度比例
constexpr double kBottomHeightRatio = 0.30;  // 底部面板高度占可用高度比例

// 面板显示名（与 Dock 标题一致：Data / Properties / Python Console）
const char* panelLabel(PanelId id) {
    switch (id) {
    case PanelId::Data:
        return QT_TRANSLATE_NOOP("PanelPreviewWidget", "Data");
    case PanelId::Property:
        return QT_TRANSLATE_NOOP("PanelPreviewWidget", "Properties");
    case PanelId::PyShell:
        return QT_TRANSLATE_NOOP("PanelPreviewWidget", "Console");
    }
    return "";
}
}  // namespace

PanelPreviewWidget::PanelPreviewWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("panelPreviewWidget"));
    setAttribute(Qt::WA_StyledBackground, true);  // 背景随 QSS（@viewBg@ 兜底）
    setMinimumSize(260, 150);
}

QSize PanelPreviewWidget::minimumSizeHint() const { return QSize(260, 150); }

void PanelPreviewWidget::paintEvent(QPaintEvent*) {
    // 主题色与全局 QSS 同源（ThemeColors 单点定义，ui-guidelines §4.5；
    // 与 004 LayoutPreviewWidget 同模式）
    const auto* theme = ThemeManager::current();
    const QColor viewBg = theme->colors.viewBg;
    const QColor panel = theme->colors.panelBg;
    const QColor border = theme->colors.border;
    const QColor textWeak = theme->colors.textWeak;

    QPainter p(this);
    p.fillRect(rect(), viewBg);

    // 几何复用纯逻辑层：与真实排布必然一致（契约 panel-settings-dialog.md §3）
    const QRect avail = rect().adjusted(kPreviewMargin, kPreviewMargin,
                                        -kPreviewMargin, -kPreviewMargin);
    if (avail.width() <= 0 || avail.height() <= 0) return;

    const bool showData = isPanelVisible(cfg_, PanelId::Data);
    const bool showProperty = isPanelVisible(cfg_, PanelId::Property);
    const bool showConsole = isPanelVisible(cfg_, PanelId::PyShell);

    // 左右面板宽度：可见时占 kSideWidthRatio，隐藏时让渡给中央区（expand 语义）
    const double dataW = showData ? kSideWidthRatio : 0.0;
    const double propertyW = showProperty ? kSideWidthRatio : 0.0;
    // 底部面板高度：可见时占 kBottomHeightRatio，隐藏时让渡
    const double consoleH = showConsole ? kBottomHeightRatio : 0.0;
    // PyShell 尺寸形态（用户示意图）：*WithConsole = 全尺寸（底部全宽，两侧面板在上方）；
    // *Only = 非全尺寸（窄条嵌入 Plot 下方，两侧面板保持全高）
    const bool fullWidthConsole = modeHasFullWidthConsole(cfg_.mode);

    const double leftX = dataW * avail.width();
    const double rightX = (1.0 - propertyW) * avail.width();
    const double bottomY = (1.0 - consoleH) * avail.height();
    // 侧面板高度：全尺寸 console 时被底部条压缩到其上方，非全尺寸时保持全高
    const double sideH = (showConsole && fullWidthConsole) ? (1.0 - consoleH) : 1.0;

    const bool dataOnRight = (targetArea(cfg_.mode, PanelId::Data) == DockArea::Right);

    const QRect leftRect(avail.left(), avail.top(),
                         qRound(dataW * avail.width()), qRound(sideH * avail.height()));
    const QRect rightRect(avail.left() + qRound(rightX), avail.top(),
                          qRound(propertyW * avail.width()), qRound(sideH * avail.height()));
    // 底部 PyShell：全尺寸 → 横跨整个可用宽度；非全尺寸 → 只占中央宽度（两端抵在两侧面板）
    const QRect bottomRect(avail.left() + (fullWidthConsole ? 0 : qRound(leftX)),
                           avail.top() + qRound(bottomY),
                           fullWidthConsole ? avail.width() : qRound(rightX - leftX),
                           qRound(consoleH * avail.height()));
    const QRect centerRect(avail.left() + qRound(leftX), avail.top(),
                           qRound(rightX - leftX), qRound(bottomY));

    p.setPen(border);
    const auto drawCell = [&p, &border](const QRect& r, const QColor& fill) {
        if (r.width() <= 0 || r.height() <= 0) return;
        p.fillRect(r, fill);
        p.drawRect(r.adjusted(0, 0, -1, -1));  // 1px 边框
    };

    // 底部先绘制（与真实 Dock 顺序一致的层次；位于左右面板之间，无重叠）
    if (showConsole) {
        drawCell(bottomRect, panel);
        p.setPen(textWeak);
        p.drawText(bottomRect, Qt::AlignCenter, tr(panelLabel(PanelId::PyShell)));
        p.setPen(border);
    }

    // 左右面板：模式决定 Data/Property 各自落位（FR-003）
    if (showData) {
        const QRect r = dataOnRight ? rightRect : leftRect;
        drawCell(r, panel);
        p.setPen(textWeak);
        p.drawText(r, Qt::AlignCenter, tr(panelLabel(PanelId::Data)));
        p.setPen(border);
    }
    if (showProperty) {
        const QRect r = dataOnRight ? leftRect : rightRect;
        drawCell(r, panel);
        p.setPen(textWeak);
        p.drawText(r, Qt::AlignCenter, tr(panelLabel(PanelId::Property)));
        p.setPen(border);
    }

    // 中央区（曲线视图）：仅描边 + 标签，保持与「中间区域」语义一致
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawRect(centerRect.adjusted(0, 0, -1, -1));
    p.setPen(textWeak);
    p.drawText(centerRect, Qt::AlignCenter, tr("Plot"));
}

}  // namespace ui
}  // namespace perception

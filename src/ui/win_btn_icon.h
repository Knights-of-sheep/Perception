// ===== 无边框窗口标题栏按钮图标（统一 16px 矢量绘制）=====
// 文本字符字形/基线不一导致观感不齐（如 □ 与 ─ 视觉高度不同），
// 故主窗口标题栏、Dock 标题栏、子窗口标题栏的按钮统一改用 QPainter
// 矢量图标：颜色随主题 palette、外观一致、等高等宽可完全对齐。
// 提供 Prev / Next 箭头供子窗口最大化循环切换使用。
#pragma once

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

namespace perception {
namespace ui {

enum class WinBtnKind {
    Minimize, Maximize, Restore, Close,
    FloatDock, Undock,
    Prev, Next,  // 子窗口最大化循环切换箭头（◀ ▶）
};

inline void drawWinBtnIcon(QPainter& p, WinBtnKind kind, const QColor& color) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    switch (kind) {
    case WinBtnKind::Minimize:
        // 横线垂直居中（与其他图标 centerY 对齐，避免按钮排布不齐平）
        p.drawLine(QPointF(3, 8.5), QPointF(13, 8.5));
        break;
    case WinBtnKind::Maximize:
        p.drawRect(QRectF(3.2, 3.2, 9.6, 9.6));  // 方框
        break;
    case WinBtnKind::Restore:
        // 两个重叠小方框（Windows 还原惯例：后框 + 前框）
        p.drawRect(QRectF(2.4, 5.4, 8.2, 8.2));
        p.drawRect(QRectF(5.4, 2.4, 8.2, 8.2));
        break;
    case WinBtnKind::Close:
        // 对角交叉
        p.drawLine(QPointF(3.6, 3.6), QPointF(12.4, 12.4));
        p.drawLine(QPointF(12.4, 3.6), QPointF(3.6, 12.4));
        break;
    case WinBtnKind::FloatDock:
        // 分离：上下对三角（⇅ 语义，停靠面板分离为浮动窗口）
        p.drawPolyline(QPolygonF() << QPointF(8, 3.2) << QPointF(4.6, 7.4)
                                   << QPointF(11.4, 7.4) << QPointF(8, 3.2));
        p.drawPolyline(QPolygonF() << QPointF(8, 12.8) << QPointF(4.6, 8.6)
                                   << QPointF(11.4, 8.6) << QPointF(8, 12.8));
        break;
    case WinBtnKind::Undock:
        // 恢复嵌入：L 形返回箭头（↩ 语义，浮动窗口回到主窗口停靠）
        p.drawLine(QPointF(12.5, 4.2), QPointF(12.5, 9.2));
        p.drawLine(QPointF(12.5, 9.2), QPointF(4.8, 9.2));
        p.drawLine(QPointF(4.8, 9.2), QPointF(7.6, 6.8));
        p.drawLine(QPointF(4.8, 9.2), QPointF(7.6, 11.6));
        break;
    case WinBtnKind::Prev:
        // 左向箭头（◀）：最大化循环切换向前
        p.drawPolyline(QPolygonF() << QPointF(8, 3.2) << QPointF(3.4, 8.0)
                                   << QPointF(8, 12.8));
        break;
    case WinBtnKind::Next:
        // 右向箭头（▶）：最大化循环切换向后
        p.drawPolyline(QPolygonF() << QPointF(8, 3.2) << QPointF(12.6, 8.0)
                                   << QPointF(8, 12.8));
        break;
    }
    p.restore();
}

inline QIcon makeWinBtnIcon(WinBtnKind kind, const QPalette& pal) {
    const QColor normal = pal.color(QPalette::WindowText);
    // 关闭按钮 hover 背景为红色（QSS @dangerHoverBg@），图标取白色保证对比；
    // 其余按钮 hover 沿用主文字色。
    const QColor active = (kind == WinBtnKind::Close)
                              ? pal.color(QPalette::BrightText)
                              : pal.color(QPalette::WindowText);
    QIcon icon;
    const int sizes[] = {16, 32};  // 兼容高 DPI 缩放
    for (int s : sizes) {
        QPixmap base(s, s);
        base.fill(Qt::transparent);
        { QPainter p(&base); drawWinBtnIcon(p, kind, normal); }
        QPixmap act(s, s);
        act.fill(Qt::transparent);
        { QPainter p(&act); drawWinBtnIcon(p, kind, active); }
        icon.addPixmap(base);                             // Normal / Off
        icon.addPixmap(act, QIcon::Active, QIcon::Off);   // hover / 按下
    }
    return icon;
}

}  // namespace ui
}  // namespace perception

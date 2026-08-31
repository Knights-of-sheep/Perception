// ===== 嵌入式 PyShell 分隔条拖拽交互单测（010-panel-layout-settings）=====
// 覆盖：非全尺寸（*Only：DualOnly / DualReversedOnly）模式下，嵌入式槽位
//   consoleEmbeddedHost_ 顶部的可拖拽分隔条（EmbeddedConsoleSash）：
//   1) 分隔条与槽位存在且可见（嵌入式宿主在 *Only 下生效）；
//   2) 拖拽方向正确（回归：曾写成 startHeight + delta，导致
//      向上拖 → PyShell 变小、向下拖 → 变大，方向完全反了）：
//      向上拖（鼠标 y 减小）→ 高度增大；向下拖（鼠标 y 增大）→ 高度减小。
// 需要 GUI 平台（QApplication + QtTest），在 Windows 桌面会话下运行。
#include "ui/MainWindow.h"
#include "ui/panellayout/panel_layout_config.h"

#include <QApplication>
#include <QMouseEvent>
#include <QWidget>

#include <cstdio>

using perception::ui::PanelLayoutConfig;
using perception::ui::PanelLayoutMode;

namespace {

// QTest::mouseMove 只移动系统光标，后续事件由窗口系统异步派发，测试不可控；
// 这里统一用同步手动事件（press/move/release 全部 sendEvent，globalPos 均基于
// "发送时刻"的 mapToGlobal）——注意不要混用 QTest::mousePress（内部 qWait 会等待
// 窗口系统处理，与直接 sendEvent 之间存在窗口定位时序差，导致 globalPos 漂移、
// 拖拽 delta 失真，测试 flaky）。
void sendDragMove(QWidget* widget, QEvent::Type type, const QPoint& local,
                  Qt::MouseButton button) {
    const QPoint global = widget->mapToGlobal(local);
    QMouseEvent ev(type, local, global, button, button, Qt::NoModifier);
    QApplication::sendEvent(widget, &ev);
}

// 返回 0 = 通过；非 0 = 失败码
int dragDirectionCheck(perception::ui::MainWindow& window) {
    auto* host = window.findChild<QWidget*>(QStringLiteral("consoleEmbeddedHost"));
    auto* sash = window.findChild<QWidget*>(QStringLiteral("consoleSash"));
    if (!host || !sash) {
        fprintf(stderr, "embedded host/sash missing\n");
        return 1;
    }
    if (!host->isVisible()) {
        fprintf(stderr, "embedded host not visible in *Only mode\n");
        return 2;
    }

    const int h0 = host->height();
    const int sx = sash->width() / 2;
    const int sy = sash->height() / 2;

    // 按下分隔条
    sendDragMove(sash, QEvent::MouseButtonPress, QPoint(sx, sy), Qt::LeftButton);
    // 向上拖 60px：鼠标 y 减小 → PyShell 应变高
    sendDragMove(sash, QEvent::MouseMove, QPoint(sx, sy - 60), Qt::LeftButton);
    QCoreApplication::processEvents();
    const int hUp = host->height();
    if (hUp <= h0) {
        fprintf(stderr, "drag up: height %d not > initial %d (direction broken)\n", hUp, h0);
        return 3;
    }

    // 从按下位置向下拖 60px：鼠标 y 增大 → PyShell 应变矮（回到初始附近）
    sendDragMove(sash, QEvent::MouseMove, QPoint(sx, sy + 60), Qt::LeftButton);
    QCoreApplication::processEvents();
    const int hDown = host->height();
    if (hDown >= hUp) {
        fprintf(stderr, "drag down: height %d not < up-drag height %d (direction broken)\n",
                hDown, hUp);
        return 4;
    }
    sendDragMove(sash, QEvent::MouseButtonRelease, QPoint(sx, sy + 60), Qt::LeftButton);
    QCoreApplication::processEvents();

    fprintf(stderr, "embedded sash drag ok: initial=%d up=%d down=%d\n", h0, hUp, hDown);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("PerceptionTest"));
    app.setApplicationName(QStringLiteral("embedded_console_sash_test"));

    {
        perception::ui::MainWindow window;
        PanelLayoutConfig cfg;
        cfg.mode = PanelLayoutMode::DualOnly;  // 非全宽嵌入式 PyShell
        window.applyPanelLayout(cfg);
        window.resize(1200, 800);
        window.show();
        QCoreApplication::processEvents();
        const int rc = dragDirectionCheck(window);
        if (rc != 0) return rc;
    }

    {
        perception::ui::MainWindow window;
        PanelLayoutConfig cfg;
        cfg.mode = PanelLayoutMode::DualReversedOnly;  // 另一非全宽形态
        window.applyPanelLayout(cfg);
        window.resize(1200, 800);
        window.show();
        QCoreApplication::processEvents();
        const int rc = dragDirectionCheck(window);
        if (rc != 0) return rc;
    }
    return 0;
}

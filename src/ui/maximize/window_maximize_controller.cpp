#include "ui/maximize/window_maximize_controller.h"

#include "ui/window_geometry.h"

#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX  // 避免与 Qt 的 qMin/qMax 及标准库 min/max 宏冲突
#endif
#include <windows.h>
#endif

namespace perception {
namespace ui {

WindowMaximizeController::WindowMaximizeController(QMainWindow* window, QObject* parent)
    : QObject(parent), window_(window) {}

bool WindowMaximizeController::handleWindowMessage(void* message, long* result) {
#ifdef Q_OS_WIN
    auto* msg = static_cast<MSG*>(message);
    if (msg->message == WM_GETMINMAXINFO) {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        // 消息于窗口尺寸改变前到达：frameGeometry 仍为最大化前几何，
        // 顺带记录供 handleWindowStateChange 还原（FR-003/006）。
        recordNormalGeometry();

        // 目标屏每次消息动态解析（不缓存屏幕指针），显示器热插拔/DPI 变化后仍正确
        //（FR-005，research R4）。
        QList<QRect> screensGeom;
        QList<QRect> screensAvail;
        const QList<QScreen*> screens = QGuiApplication::screens();
        screensGeom.reserve(screens.size());
        screensAvail.reserve(screens.size());
        for (const QScreen* scr : screens) {
            screensGeom.append(scr->geometry());
            screensAvail.append(scr->availableGeometry());
        }
        const int idx = window_geometry::resolveTargetScreenIndex(screensGeom, normalGeometry_);
        // ptMaxPosition 为相对目标屏幕左上角的偏移（Windows MINMAXINFO 语义，
        // research R6）：直接填虚拟桌面绝对坐标会把窗口推到目标屏之外
        //（副屏最大化"消失"根因，2026-08-29 复核）。
        const auto info = window_geometry::maximizeInfo(screensGeom, screensAvail, idx);
        if (!info.maxSize.isEmpty()) {
            mmi->ptMaxPosition.x = info.maxPosition.x();
            mmi->ptMaxPosition.y = info.maxPosition.y();
            mmi->ptMaxSize.x = info.maxSize.width();
            mmi->ptMaxSize.y = info.maxSize.height();
        }
        *result = 0;
        return true;
    }
#else
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

void WindowMaximizeController::toggleMaximize() {
    if (isMaximized()) {
        // 005-multi-screen-maximize：几何恢复统一由 handleWindowStateChange
        //（WindowStateChange）完成（按钮/双击/任务栏右键/快捷键同一路径，FR-003/006）
        window_->showNormal();
    } else {
        window_->showMaximized();
    }
}

bool WindowMaximizeController::isMaximized() const {
    return window_->isMaximized();
}

void WindowMaximizeController::recordNormalGeometry() {
    normalGeometry_ = window_->frameGeometry();
    normalGeometryValid_ = !normalGeometry_.isEmpty();
}

void WindowMaximizeController::handleWindowStateChange() {
    // normalGeometry_ 已在 WM_GETMINMAXINFO 记录（消息于窗口尺寸改变前到达，
    // frameGeometry 为最大化前几何，准确）；此处只做过渡判定与系统侧触发的还原：
    //   - 进入最大化：确保已记录（兜底取当前几何，覆盖极少数未走该消息的路径）
    //   - 退出最大化：还原最大化前几何（按钮/双击/任务栏右键/快捷键统一路径，
    //     原屏原尺寸，FR-003；若原屏已被断开，由系统将窗口归位到可见区域）
    const bool maximized = isMaximized();
    if (maximized && !prevMaximized_) {
        if (!normalGeometryValid_) recordNormalGeometry();
    } else if (!maximized && prevMaximized_ && normalGeometryValid_ &&
               !normalGeometry_.isEmpty()) {
        window_->setGeometry(normalGeometry_);
        normalGeometryValid_ = false;  // 一次性还原，再次最大化时由 MINMAXINFO 重新记录
    }
    prevMaximized_ = maximized;
    emit maximizedChanged(maximized);
}

}  // namespace ui
}  // namespace perception

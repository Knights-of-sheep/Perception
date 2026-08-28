// ===== 窗口最大化几何计算：纯函数实现（005-multi-screen-maximize）=====
// 实现见 window_geometry.h 的接口契约（research R2 / contracts §2）。
// 不依赖 QWidget*/QApplication 实例，tests/cpp 无 GUI 环境直接单测。
#include "ui/window_geometry.h"

namespace perception {
namespace ui {
namespace window_geometry {

int resolveTargetScreenIndex(const QList<QRect>& screensGeom, const QRect& windowFrame) {
    if (screensGeom.isEmpty()) {
        return -1;
    }
    const QPoint center = windowFrame.center();
    // 窗口中心命中的屏幕优先（跨屏边界按中心归属，contract §2）
    for (int i = 0; i < screensGeom.size(); ++i) {
        if (screensGeom.at(i).contains(center)) {
            return i;
        }
    }
    // 未命中（窗口中心落在屏幕间缝隙/任务栏等）→ 回退首个屏幕
    return 0;
}

MaximizeInfo maximizeInfo(const QList<QRect>& screensGeom,
                          const QList<QRect>& screensAvailGeom,
                          int index) {
    if (screensGeom.isEmpty() || screensAvailGeom.isEmpty() ||
        screensGeom.size() != screensAvailGeom.size()) {
        return MaximizeInfo{};
    }
    // ptMaxPosition 为相对目标屏幕左上角的偏移（Windows MINMAXINFO 语义，
    // research R6）：avail 工作区左上角 − 目标屏完整几何左上角。
    auto make = [](const QRect& geom, const QRect& avail) {
        MaximizeInfo info;
        info.maxPosition =
            QPoint(avail.left() - geom.left(), avail.top() - geom.top());
        info.maxSize = avail.size();
        return info;
    };
    if (index >= 0 && index < screensGeom.size() &&
        !screensGeom.at(index).isEmpty() && !screensAvailGeom.at(index).isEmpty()) {
        return make(screensGeom.at(index), screensAvailGeom.at(index));
    }
    // index 越界或对应项为空（如该屏已断开）→ 回退首个两列表均非空的对
    for (int i = 0; i < screensGeom.size(); ++i) {
        if (!screensGeom.at(i).isEmpty() && !screensAvailGeom.at(i).isEmpty()) {
            return make(screensGeom.at(i), screensAvailGeom.at(i));
        }
    }
    return MaximizeInfo{};
}

}  // namespace window_geometry
}  // namespace ui
}  // namespace perception

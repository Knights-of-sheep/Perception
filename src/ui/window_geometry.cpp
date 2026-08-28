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

QRect maximizeGeometry(const QList<QRect>& screensAvailGeom, int index) {
    if (screensAvailGeom.isEmpty()) {
        return QRect();
    }
    if (index >= 0 && index < screensAvailGeom.size() && !screensAvailGeom.at(index).isEmpty()) {
        return screensAvailGeom.at(index);
    }
    // index 越界或对应项为空（如该屏已断开）→ 回退首个非空项
    for (const QRect& r : screensAvailGeom) {
        if (!r.isEmpty()) {
            return r;
        }
    }
    return QRect();
}

}  // namespace window_geometry
}  // namespace ui
}  // namespace perception

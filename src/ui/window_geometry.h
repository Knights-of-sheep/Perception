// ===== 窗口最大化几何计算（005-multi-screen-maximize）=====
// 无边框主窗口在任意显示器上最大化的纯函数层：
//   - 输入为屏幕几何数据（DIP），不依赖 QWidget*/QApplication 实例，
//     可在 tests/cpp 无 GUI 环境直接单测（宪法 II Test-First，research R5）。
//   - 接线层（MainWindow::nativeEvent 的 WM_GETMINMAXINFO）负责从
//     QGuiApplication::screens() 与窗口 frameGeometry 提取数据后调用本层
//     （research R2 / contracts/window-maximize-contract.md §2）。
#pragma once

#include <QList>
#include <QRect>

namespace perception {
namespace ui {
namespace window_geometry {

// 解析窗口"主体所在"屏幕下标：
//   screensGeom —— 候选屏幕完整几何（QScreen::geometry()，含任务栏/系统区），
//                  用于命中判定（与窗口中心点比较）；
//   windowFrame —— 窗口 frameGeometry（DIP）。
// 行为（contract §2）：窗口中心点命中的屏幕下标优先；窗口中心落在所有屏幕几何
// 之外（屏幕间缝隙/任务栏等）或列表为空 → 回退首个屏幕（下标 0）；列表完全为空
// 时返回 -1（调用方跳过限制）。
int resolveTargetScreenIndex(const QList<QRect>& screensGeom, const QRect& windowFrame);

// 最大化几何：目标屏幕的可用工作区（QScreen::availableGeometry()）。
//   screensAvailGeom —— 与 screensGeom 一一对应的可用几何列表；
//   index             —— resolveTargetScreenIndex() 的结果。
// index 合法且非空 → 返回对应项；index 越界/对应项为空 → 回退列表首个非空项；
// 列表全空 → QRect()（调用方保留原行为，不覆写 MINMAXINFO）。
QRect maximizeGeometry(const QList<QRect>& screensAvailGeom, int index);

}  // namespace window_geometry
}  // namespace ui
}  // namespace perception

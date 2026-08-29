// ===== 多屏最大化/还原控制器（006-constitution-refactor：自 MainWindow 提取）=====
// 005-multi-screen-maximize（FR-003/006）：无边框主窗口在任意显示器上最大化、
// 还原时保持原屏原尺寸。职责：
//   - toggleMaximize：按钮/双击标题栏统一入口；
//   - handleWindowStateChange：最大化过渡几何管理（MainWindow::changeEvent 转发）；
//   - handleWindowMessage：WM_GETMINMAXINFO 目标屏工作区限制（nativeEvent 转发）。
// 不持有窗口 UI 元素：最大化/还原按钮图标刷新由 MainWindow 连接 maximizedChanged
// 信号完成（低耦合）。
#pragma once

#include <QObject>
#include <QRect>

class QMainWindow;

namespace perception {
namespace ui {

class WindowMaximizeController : public QObject {
    Q_OBJECT

public:
    explicit WindowMaximizeController(QMainWindow* window, QObject* parent = nullptr);

    // 处理窗口原生消息：当前支持 Windows WM_GETMINMAXINFO（限制最大化到窗口"主体所在"
    // 屏幕的工作区）。返回 true 表示已处理（结果已写入 result）。非 Windows 恒返回 false。
    bool handleWindowMessage(void* message, long* result);

    // 最大化/还原切换（按钮/双击标题栏统一入口）。
    void toggleMaximize();

    // 窗口状态变化（MainWindow::changeEvent 转发）：管理最大化过渡与几何还原。
    // 覆盖系统侧触发（任务栏右键、快捷键）与按钮触发。
    void handleWindowStateChange();

    bool isMaximized() const;

signals:
    // 最大化状态变化（MainWindow 据此刷新最大化/还原按钮图标）。
    void maximizedChanged(bool maximized);

private:
    void recordNormalGeometry();  // 记录最大化前几何（frameGeometry 于消息到达时未变）

    QMainWindow* window_ = nullptr;
    QRect normalGeometry_;
    bool normalGeometryValid_ = false;
    bool prevMaximized_ = false;
};

}  // namespace ui
}  // namespace perception

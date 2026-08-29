// ===== Dock 拖拽 / 分隔条拖拽高亮控制器（006-constitution-refactor：自 MainWindow 提取）=====
// VSCode 风格分割线指示，职责：
//   - Dock 拖拽（DockTitleBar 回调驱动）：全窗口高亮覆盖层，绘制"目标区域半透明填充 +
//     分割线 3px 实线"，放置时按鼠标所在放置带移动 dock；
//   - 分隔条（dock 边缘分隔条）resize 拖拽（MainWindow::event 驱动）：细条高亮 overlay，
//     只覆盖分隔条缝隙，局部重绘不卡顿。
// 自身不拦截事件，由 MainWindow 的事件转发驱动。
#pragma once

#include <QObject>
#include <QRect>

class QDockWidget;
class QMainWindow;
class QPoint;
class QWidget;

namespace perception {
namespace ui {

class DockDragOverlay : public QObject {
    Q_OBJECT

public:
    explicit DockDragOverlay(QMainWindow* window, QObject* parent = nullptr);

    // ---- Dock 拖拽高亮（由 DockTitleBar::eventFilter 回调驱动）----
    void beginDockDrag(QDockWidget* dock);         // 进入拖拽：显示高亮覆盖层
    void updateDockDrag(const QPoint& globalPos);  // 拖拽中：按鼠标位置更新高亮目标
    void endDockDrag(const QPoint& globalPos);     // 结束：执行放置并隐藏高亮

    // ---- 分隔条（dock 边缘分隔条）resize 拖拽高亮（由 MainWindow::event 驱动）----
    void beginSashDrag(int hit);  // 按住分隔条：进入高亮
    void updateSashDrag();        // 拖拽中/布局变化后：同步高亮条到分隔条
    void endSashDrag();           // 松开：隐藏高亮条

    // 命中检测：主窗口局部坐标是否落在某条真实分隔条上（返回 SashHit，未命中返回 0）
    int hitTest(const QPoint& pos) const;
    // 当前高亮线矩形（由命中类型 + 最新 dock geometry 计算；分隔条拖拽高亮用）
    QRect highlightRect() const;
    // 覆盖层跟随主窗口尺寸（MainWindow::resizeEvent 转发）
    void syncToWindowSize();
    // 分隔条拖拽进行中（MainWindow::event 据此判断鼠标释放时结束高亮）
    bool isSashDragging() const { return sashDragging_; }

    // 分隔条 = 相邻 dock/中央区域之间 1px 边界线（由 Qt 绘制在 QMainWindow 上，
    // 非独立 widget）。命中类型区分左右/上下多条分隔条。
    enum SashHit {
        SashMiss = 0,
        SashFileRight,    // 垂直分隔条：左侧 fileDock 右缘
        SashPropertyLeft, // 垂直分隔条：右侧 propertyDock 左缘
        SashPythonTop,    // 水平分隔条：底部 pythonDock 上缘
    };

private:
    QDockWidget* fileDock() const;      // findChild 定位（与 QSS/布局记忆 objectName 一致）
    QDockWidget* propertyDock() const;
    QDockWidget* pythonDock() const;

    QMainWindow* window_ = nullptr;
    QWidget* dragOverlayWidget_ = nullptr;      // 全窗口覆盖层（鼠标穿透，绘制分割线高亮）
    QWidget* sashHighlightWidget_ = nullptr;    // 分界线高亮细条（仅覆盖分隔条缝隙，亮色）
    QDockWidget* dragDock_ = nullptr;           // 正在拖拽的 dock（endDockDrag 执行放置）
    bool sashDragging_ = false;
    int sashHit_ = SashMiss;  // 当前拖拽命中的分隔条类型
};

}  // namespace ui
}  // namespace perception

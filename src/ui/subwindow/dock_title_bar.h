// ===== DockTitleBar：Dock 自定义标题栏 =====
// 背景：Qt5 + Fusion 风格下，浮动的 QDockWidget 不绘制 normal-button 子控件，
//       导致"恢复嵌入"按钮缺失，用户分离后找不到还原入口。
// 方案：setTitleBarWidget(DockTitleBar) 接管标题栏——浮动时彻底无系统标题栏
//       （Qt::FramelessWindowHint），标题栏自身承担拖动移动 / 双击最大化/还原；
//       按钮集：停靠态 = 最大化 + 分离 + 关闭；浮动态 = 最小化 + 最大化/还原 +
//       恢复嵌入 + 关闭，全用显式 QToolButton，停靠/浮动均稳定可见。
// 006-constitution-refactor：自 MainWindow.cpp 提取（Dock 装配职责下放）。
// 附带 Dock 辅助（与 DockTitleBar 一起随 createDocks 复用）：
//   - NoFocusRectDockStyle / applyNoFocusRectStyle：抑制 dock 蓝色 focus 边框
//   - wrapWithSizeGrip：浮动窗口右下角调整大小手柄（QSizeGrip 补位）
#pragma once

#include <QProxyStyle>
#include <QWidget>

class QDockWidget;
class QLabel;
class QMouseEvent;
class QToolButton;

namespace perception {
namespace ui {

class MainWindow;

// Dock 自定义标题栏：停靠/浮动两种状态切换按钮集与拖拽行为
class DockTitleBar : public QWidget {
public:
    explicit DockTitleBar(QDockWidget* parent);
    // 010-panel-layout-settings：PyShell 嵌入式模式的"全宽覆盖"态——
    // active=true 时 maxBtn 显示"恢复嵌入式"图标，点击回嵌入式窄条宿主（setConsoleFullWidth(false)）。
    void setFullWidthConsole(bool active);

protected:
    // 停靠态：press→超过阈值→Dragging，通知主窗口显示放置高亮；release 执行放置。
    // 浮动态：press/move 直接移动浮动窗口。
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void changeEvent(QEvent* e) override;

private:
    QToolButton* makeDockBtn(const QString& objName);
    void refreshByState();
    void updateFloatIcon(bool floating);
    void refreshIcons();
    void refreshMaxBtn();
    void onTopLevelChanged(bool topLevel);
    MainWindow* mainWindow() const;  // 所属主窗口（拖拽高亮回调目标）

    QDockWidget* dock_;
    QLabel*       titleLabel_;
    QToolButton*  minBtn_;
    QToolButton*  maxBtn_;
    QToolButton*  floatBtn_;
    QToolButton*  closeBtn_;

    // 自定义拖拽状态（停靠=分割线高亮；浮动=窗口移动）
    enum class DragState { None, Candidate, Dragging, FloatMove };
    DragState dragState_ = DragState::None;
    QPoint    pressGlobal_;
    QPoint    winPos_;  // 浮动态拖动前的窗口位置

    bool fullWidthConsoleOverride_ = false;  // 010：PyShell 全宽覆盖态（maxBtn 行为切换）
};

// Dock focus rect 抑制（QProxyStyle）：Qt Fusion 风格在 QDockWidget 获得键盘焦点时
// 自动绘制 1-2px focus rect（QStyle::PE_FrameFocusRect），包裹整个 dock 外缘——
// 视觉上像"dock 被高亮选中"，与设计语言不符。仅对 QDockWidget 抑制该元素。
class NoFocusRectDockStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                       QPainter* p, const QWidget* w) const override;
};

void applyNoFocusRectStyle(QDockWidget* dock);

// 浮动窗口右下角调整大小手柄（去系统标题栏后失去边缘 resize，QSizeGrip 补位）
QWidget* wrapWithSizeGrip(QWidget* content, QDockWidget* dock);

}  // namespace ui
}  // namespace perception

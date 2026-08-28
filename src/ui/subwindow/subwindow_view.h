// ===== 单个渲染子窗口（004-dock-layout-manager）=====
// 普通 QWidget 子控件（无系统标题栏），顶部自定义标题区（research.md 第 3 节）：
// 标题 + 按钮集（全部为等高等宽的矢量图标按钮，见 win_btn_icon.h）。
//   - 非最大化、非隐藏：最大化、隐藏、关闭
//   - 最大化：还原、向前切换、向后切换、关闭
// 最大化 = 选中子窗口占满容器（中央区域）；全屏 = 中央区域扩展至整个主界面
// （"视图"菜单触发，见 MainWindow），与子窗口最大化正交组合。
// 内容区为占位渲染视图（FR-003），视图状态随子窗口存续（FR-019）。
#pragma once

#include <QFrame>
#include <QPointer>
#include <QSize>
#include <QString>

class QContextMenuEvent;
class QEvent;
class QLabel;
class QMouseEvent;
class QToolButton;
class QVBoxLayout;

namespace perception {
namespace ui {

// 继承 QFrame：边框/背景由 paintEvent 自绘（QPainter）接管，不依赖 QStyleSheetStyle
// 对 QFrame border 宽度的 lineWidth 钳制（QSS 仅作语义参考，见 theme_template.qss 注释）。
class SubwindowView : public QFrame {
    Q_OBJECT

public:
    explicit SubwindowView(const QString& title, QWidget* parent = nullptr);

    QString title() const { return title_; }
    void setTitle(const QString& title);

    // "保持相同宽高"开启前的独立尺寸（FR-010，关闭开关时恢复）
    QSize sizeBeforeSameSize() const { return sizeBeforeSameSize_; }
    void setSizeBeforeSameSize(const QSize& s) { sizeBeforeSameSize_ = s; }

    // 容器侧在最大化状态变化时调用：更新标题区按钮显隐。
    //   maximized=true：显示 还原/向前/向后；否则显示 最大化/隐藏。
    //   （关闭按钮始终显示）
    void setMaximizedState(bool maximized);

signals:
    void selected();            // 单击子窗口任意区域（spec Assumptions：选中的子窗口）
    void maximizeRequested();   // 标题区最大化按钮 / 双击标题区（FR-016）
    void hideRequested();       // 标题区隐藏按钮（从布局中隐藏，可经 View 菜单恢复）
    void restoreRequested();    // 标题区还原按钮（退出最大化）
    void prevRequested();       // 最大化时向前切换（上一子窗口）
    void nextRequested();       // 最大化时向后切换（下一子窗口）
    void closeRequested();      // 标题区关闭按钮（销毁该子窗口）

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;  // 标题栏双击
    void mousePressEvent(QMouseEvent* event) override;           // 单击选中
    void contextMenuEvent(QContextMenuEvent* event) override;    // 右键菜单
    void paintEvent(QPaintEvent* event) override;  // 自绘：panelBg 背景 + 1px/2px @border@ 边框
    bool event(QEvent* event) override;            // 主题热切换（PaletteChange）→ 重绘刷新颜色

private:
    void createUi();

    QString title_;
    QSize sizeBeforeSameSize_;

    QWidget* titleBar_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QToolButton* maximizeBtn_ = nullptr;
    QToolButton* hideBtn_ = nullptr;
    QToolButton* restoreBtn_ = nullptr;
    QToolButton* prevBtn_ = nullptr;
    QToolButton* nextBtn_ = nullptr;
    QToolButton* closeBtn_ = nullptr;
    QWidget* content_ = nullptr;  // 占位渲染视图
    bool maximized_ = false;
};

}  // namespace ui
}  // namespace perception

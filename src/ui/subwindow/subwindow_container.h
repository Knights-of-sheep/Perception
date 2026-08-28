// ===== 中央区域子窗口容器（004-dock-layout-manager）=====
// 持有全部子窗口，按 LayoutManager 计算执行排列（QGridLayout 重建）。
// 支持：单子窗口铺满、最大化（容器内独占，其余隐藏，FR-016）、
// 隐藏（标题栏"隐藏"按钮，可从布局移除且可经 View 菜单恢复）、
// 最大化窗口的向前/向后循环切换、空状态提示。
// 全屏协调由 MainWindow 负责（隐藏三个 Dock 使容器扩展占满整个主界面）。
#pragma once

#include <QVector>
#include <QWidget>

#include "ui/subwindow/layout_manager.h"

class QGridLayout;
class QLabel;

namespace perception {
namespace ui {

class SubwindowView;

class SubwindowContainer : public QWidget {
    Q_OBJECT

public:
    explicit SubwindowContainer(QWidget* parent = nullptr);

    // 子窗口管理：加入/移除并立即重排（FR-001/002）
    void addSubwindow(SubwindowView* view);
    void removeSubwindow(SubwindowView* view);
    QVector<SubwindowView*> subwindows() const { return views_; }
    int subwindowCount() const { return views_.size(); }
    SubwindowView* maximizedView() const { return maximized_; }

    // 布局配置：修改即重排（FR-011 即时生效）
    void setLayoutConfig(const LayoutConfig& cfg);
    const LayoutConfig& layoutConfig() const { return cfg_; }

    // 最大化：容器内仅显示选中子窗口，其余隐藏；再次调用/exit 恢复原排列（FR-016）。
    // 最大化进出不销毁子窗口，内容与视图状态保留（FR-019）。
    void setMaximized(SubwindowView* view);
    void exitMaximized();
    bool isMaximized() const { return maximized_ != nullptr; }

    // 隐藏：从当前排列中移除（不销毁），可经 showHiddenSubwindows 恢复。
    void hideSubwindow(SubwindowView* view);
    void showHiddenSubwindows();
    int hiddenSubwindowCount() const { return hidden_.size(); }

    // 最大化状态下向前（direction=-1）/向后（direction=+1）循环切换最大化对象。
    void cycleMaximized(int direction);

    // 无子窗口时显示空状态提示
    void setEmptyHint(const QString& text);

    // 按当前配置重新排列（MainWindow 全屏退出恢复时亦调用）
    void relayout();

signals:
    void subwindowCountChanged(int count);

private:
    void syncSameSize(bool enabled);

    LayoutManager layoutManager_;
    LayoutConfig cfg_;
    QVector<SubwindowView*> views_;
    QVector<SubwindowView*> hidden_;  // 用户通过"隐藏"按钮隐藏的子窗口（不参与排列，不销毁）
    SubwindowView* maximized_ = nullptr;
    QLabel* emptyHint_ = nullptr;
    QGridLayout* grid_ = nullptr;
};

}  // namespace ui
}  // namespace perception

// ===== 布局设置实时排列预览（008-unify-dialog-styling WS2，FR-013）=====
// 自绘 QWidget：输入可见子窗口数 + 当前 LayoutConfig，复用 LayoutManager::computeGrid /
// cellRect（已测纯函数）绘制 cell 缩略示意——预览与真实排布必然一致（契约 §6）。
// 主题色取自 ThemeManager 当前色板（viewBg 背景 / panelBg cell / border 1px），
// 与全局 QSS 同源；主题热切换后随重绘自动跟随。
// 空态：n=0 显示提示文本；n=1 单格铺满（Edge Cases）。
#pragma once

#include <QWidget>

#include "ui/subwindow/layout_manager.h"

namespace perception {
namespace ui {

class LayoutPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit LayoutPreviewWidget(QWidget* parent = nullptr);

    // 可见子窗口数（SubwindowContainer::visibleSubwindowCount 提供）与当前布局配置；
    // 任一变化即重绘。
    void setPreviewCount(int count);
    void setConfig(const LayoutConfig& cfg);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int previewCount_ = 0;
    LayoutConfig cfg_;
};

}  // namespace ui
}  // namespace perception

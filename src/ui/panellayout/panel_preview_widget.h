// ===== 面板布局实时示意图（010-panel-layout-settings FR-006）=====
// 对话框内自绘布局示意图：几何复用 PanelLayoutConfig（targetArea / isPanelVisible），
// 与真实排布必然一致（契约 panel-settings-dialog.md §3「预览真实一致」）。
// objectName panelPreviewWidget（QSS 背景兜底，仿 004 LayoutPreviewWidget）。
#pragma once

#include <QWidget>

#include "ui/panellayout/panel_layout_config.h"

namespace perception {
namespace ui {

class PanelPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PanelPreviewWidget(QWidget* parent = nullptr);

    // 更新配置并重绘（模式 × 显隐）
    void setConfig(const PanelLayoutConfig& cfg) {
        cfg_ = cfg;
        update();
    }

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    PanelLayoutConfig cfg_;
};

}  // namespace ui
}  // namespace perception

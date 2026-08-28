// ===== 布局设置界面（004-dock-layout-manager，US2/3/4/5）=====
// 统一布局设置入口：排列模式 + 最大行/列 + 优先行/优先列 + 保持相同宽高。
// 修改即时生效（FR-011），非法输入由 QSpinBox 范围（0–10）天然拦截（FR-012）。
// 打开时回显当前配置（US5）。
// 弹窗风格与其他弹窗一致：无边框 + 自定义标题栏（图标+标题+关闭按钮，可拖拽，
// FR-011 / 宪法「技术栈约束 · GUI」），由 buildDialogTitleBar 提供。
// 按模式显隐约束控件：Grid 显示 最大行/最大列 + 优先行/优先列（radio 组）；
// 优先行排（By Row）仅显示 最大行；优先列排（By Column）仅显示 最大列。
#pragma once

#include <QDialog>

#include <QPoint>

#include "ui/subwindow/layout_manager.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QMouseEvent;
class QRadioButton;
class QSpinBox;
class QWidget;

namespace perception {
namespace ui {

class LayoutSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit LayoutSettingsDialog(const LayoutConfig& initial, QWidget* parent = nullptr);

    // 回显/更新当前配置（US5：打开时展示当前各值）
    void setConfig(const LayoutConfig& cfg) { syncFrom(cfg); }

signals:
    // 任一控件修改即发射，由 MainWindow 转发至容器立即重排（FR-011）
    void configChanged(const LayoutConfig& cfg);

protected:
    // 无边框自定义标题栏拖拽移动（与其他弹窗一致，FR-011）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void emitConfig();
    void updateConstraintVisibility();
    LayoutConfig current() const;
    void syncFrom(const LayoutConfig& cfg);

    QComboBox* modeCombo_ = nullptr;
    QLabel* maxColsLabel_ = nullptr;
    QSpinBox* maxColsSpin_ = nullptr;
    QLabel* maxRowsLabel_ = nullptr;
    QSpinBox* maxRowsSpin_ = nullptr;
    QCheckBox* sameSizeCheck_ = nullptr;
    QLabel* priorityLabel_ = nullptr;
    QRadioButton* priorityRowRadio_ = nullptr;
    QRadioButton* priorityColRadio_ = nullptr;
    QWidget* titleBar_ = nullptr;
    QPoint dragOffset_;
};

}  // namespace ui
}  // namespace perception

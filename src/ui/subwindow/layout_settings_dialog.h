// ===== 布局设置界面（004-dock-layout-manager US2/3/4/5；008 WS2 重构）=====
// 统一布局设置入口：排列模式（分段按钮组）+ 优先级 + 最大行/列 + 保持相同宽高 +
// 间隙宽度 + 实时排列预览 + 恢复默认。
// 修改即时生效（FR-011），非法输入由 QSpinBox 范围天然拦截（FR-012）。
// 打开时回显当前配置（US5）。
// 弹窗风格与其他弹窗一致：无边框 + 自定义标题栏（图标+标题+关闭按钮，可拖拽，
// FR-011 / 宪法「技术栈约束 · GUI」），由 buildDialogTitleBar 提供。
// 控件可用性以 LayoutManager::constraintAxis 为单一判定源（008 契约 §2/§3，2026-08-30
// 二次修订：约束轴 = 生效轴）：
//   By Row / By Column：禁用 优先级 + 最大行 + 最大列（约束不参与计算，FR-008）；
//   Grid + 行优先：启用 优先级 + 仅最大列数，灰显最大行数；
//   Grid + 列优先：启用 优先级 + 仅最大行数，灰显最大列数。
// 所有控件始终可见、仅切换 enabled——切换不改变弹窗几何，避免模式/优先级切换造成布局
// 跳动（SC-006）。禁用控件当前值保留在 LayoutConfig 中但不参与计算（Edge Case）。
#pragma once

#include <QDialog>

#include <QPoint>

#include "ui/subwindow/layout_manager.h"

class QButtonGroup;
class QCheckBox;
class QLabel;
class QMouseEvent;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QWidget;

namespace perception {
namespace ui {

class LayoutPreviewWidget;

class LayoutSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit LayoutSettingsDialog(const LayoutConfig& initial, QWidget* parent = nullptr);

    // 回显/更新当前配置（US5：打开时展示当前各值）
    void setConfig(const LayoutConfig& cfg) { syncFrom(cfg); }

    // 预览计数：可见子窗口数（FR-013；MainWindow 打开时与 subwindowCountChanged 同步）
    void setPreviewCount(int count);

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
    // 恢复默认配置（FR-014）并同步控件 + 发射 configChanged
    void restoreDefaults();

    QButtonGroup* modeGroup_ = nullptr;
    QPushButton* gridButton_ = nullptr;   // 分段按钮组：Grid / By Row / By Column
    QPushButton* rowButton_ = nullptr;
    QPushButton* colButton_ = nullptr;
    QLabel* maxColsLabel_ = nullptr;
    QSpinBox* maxColsSpin_ = nullptr;
    QLabel* maxRowsLabel_ = nullptr;
    QSpinBox* maxRowsSpin_ = nullptr;
    QCheckBox* sameSizeCheck_ = nullptr;
    QLabel* priorityLabel_ = nullptr;
    QRadioButton* priorityRowRadio_ = nullptr;
    QRadioButton* priorityColRadio_ = nullptr;
    QLabel* gapLabel_ = nullptr;
    QSpinBox* gapSpin_ = nullptr;  // 间隙宽度（FR-015：0–50 px，默认 6）
    QPushButton* restoreButton_ = nullptr;
    LayoutPreviewWidget* preview_ = nullptr;
    QWidget* titleBar_ = nullptr;
    QPoint dragOffset_;
};

}  // namespace ui
}  // namespace perception

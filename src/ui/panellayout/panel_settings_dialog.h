// ===== 面板布局设置对话框（010-panel-layout-settings US1~US3）=====
// 统一布局设置入口（View 菜单 → Panel Settings，FR-001）：
//   - 四种预设布局模式（QButtonGroup exclusive，objectName panelModeButton，QSS 样式名；
//     FR-002：模式 = 左右分配 × PyShell 尺寸形态——*Only 非全尺寸（嵌入 Plot 下方窄条）、
//     *WithConsole 全尺寸（底部全宽）；弹窗排列：上排非全尺寸、下排全尺寸）
//   - 三面板独立显隐（panelDataVisibleCheck / panelPropertyVisibleCheck /
//     panelConsoleVisibleCheck，FR-004）
//   - 实时示意图（panelPreviewWidget，FR-006）+ 恢复默认（panelResetDefaultsButton）
//   - OK（panelOkButton）：持久化四 key 并关闭（FR-007，US1 场景 3）；
//     Cancel / 标题栏关闭（panelCancelButton / 标题栏关闭按钮）：恢复打开前快照并重放，
//     主窗口回滚到打开前布局（US3 场景 3，contracts/panel-settings-dialog.md §3）。
// 任一控件变更即发射 configChanged（US3 实时预览；MainWindow 转发 applyPanelLayout）。
// 弹窗风格与其他弹窗一致：无边框 + 自定义标题栏（buildDialogTitleBar，可拖拽移动）。
#pragma once

#include <QDialog>

#include <QPoint>

#include "ui/panellayout/panel_layout_config.h"

class QButtonGroup;
class QCheckBox;
class QMouseEvent;
class QPushButton;
class QWidget;

namespace perception {
namespace ui {

class PanelPreviewWidget;

class PanelSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PanelSettingsDialog(const PanelLayoutConfig& initial, QWidget* parent = nullptr);

    // 回显/更新当前配置（US1：打开时展示当前生效配置；不触发重放）。
    // 重置去重状态：单例对话框重开时，允许与上次会话相同的配置再次发射。
    void setConfig(const PanelLayoutConfig& cfg) {
        firstEmitDone_ = false;
        syncFrom(cfg);
    }
    // 当前控件合成的配置（测试断言 / OK 持久化用）
    PanelLayoutConfig current() const;

signals:
    // 任一控件修改即发射（US3 实时预览；Cancel 回滚时重放快照）。
    // 按值传递：自定义类型 + const 引用会让 QSignalSpy/QMetaType 按名字查找失败
    // （moc 参数类型串为 "const ...&"），按值可被运行时元类型系统正确反序列化。
    void configChanged(PanelLayoutConfig cfg);

public slots:
    void accept() override;  // OK：持久化四 key（FR-007）并关闭

protected:
    // Cancel / 标题栏关闭：恢复打开前快照并发射（主窗口回滚，US3 场景 3）
    void reject() override;
    // 无边框自定义标题栏拖拽移动（参考 layout_settings_dialog：仅标题栏区域可拖，
    // 与其他弹窗一致，FR-011 / 宪法「技术栈约束 · GUI」）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void emitConfig();
    // 模式按钮点击（QButtonGroup::idClicked，携带点击按钮 id；不依赖 checkedId 时序）
    void emitConfigForMode(int id);
    void restoreDefaults();
    void syncFrom(const PanelLayoutConfig& cfg);

    QButtonGroup* modeGroup_ = nullptr;
    QPushButton* dualOnlyButton_ = nullptr;
    QPushButton* dualWithConsoleButton_ = nullptr;
    QPushButton* dualReversedOnlyButton_ = nullptr;
    QPushButton* dualReversedWithConsoleButton_ = nullptr;
    QCheckBox* dataCheck_ = nullptr;
    QCheckBox* propertyCheck_ = nullptr;
    QCheckBox* consoleCheck_ = nullptr;
    QPushButton* restoreButton_ = nullptr;
    QPushButton* okButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    PanelPreviewWidget* preview_ = nullptr;
    QWidget* titleBar_ = nullptr;  // 自定义标题栏（拖拽命中区域）
    QPoint dragOffset_;            // 拖拽偏移（按下时全局位置 - 窗口左上角）
    PanelLayoutConfig cfg0_;  // 打开前快照（US3 Cancel 回滚目标）
    PanelLayoutConfig lastEmittedConfig_;  // 上次发射的配置（exclusive 双 toggle 去重）
    bool firstEmitDone_ = false;           // 是否已发生过首次发射
};

}  // namespace ui
}  // namespace perception

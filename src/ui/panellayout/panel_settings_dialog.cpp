// ===== 面板布局设置对话框实现（010-panel-layout-settings US1~US3）=====
#include "ui/panellayout/panel_settings_dialog.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "ui/dialog_title_bar.h"
#include "ui/panellayout/panel_preview_widget.h"

namespace perception {
namespace ui {

namespace {
// 对话框最小宽度（模式按钮 2x2 网格 + 显隐开关 + 预览图的紧凑排布）
constexpr int kDialogWidth = 360;
}  // namespace

PanelSettingsDialog::PanelSettingsDialog(const PanelLayoutConfig& initial, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint), cfg0_(initial) {
    setObjectName(QStringLiteral("panelSettingsDialog"));
    setWindowTitle(tr("Panel Settings"));
    // 与其他弹窗（layout_settings_dialog 等）一致的框架：窗口图标 + 无边框自定义标题栏
    setWindowIcon(QApplication::windowIcon());
    // 最小宽度（模式按钮 2x2 网格 + 显隐开关 + 预览图；参考 layout_settings 的 setMinimumWidth，
    // 高度由内容自适应）
    setMinimumWidth(kDialogWidth);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 无边框自定义标题栏（图标 + 标题 + 关闭按钮，可拖拽；关闭 → close → reject → 回滚）
    titleBar_ = buildDialogTitleBar(this, tr("Panel Settings"));
    root->addWidget(titleBar_);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(16, 14, 16, 16);
    bodyLayout->setSpacing(10);

    // ---- 模式区（FR-002：四种预设布局模式，互斥）----
    bodyLayout->addWidget(new QLabel(tr("Layout Mode"), body));

    modeGroup_ = new QButtonGroup(this);
    modeGroup_->setExclusive(true);
    auto* modeGrid = new QGridLayout();
    modeGrid->setHorizontalSpacing(8);
    modeGrid->setVerticalSpacing(8);

    dualOnlyButton_ = new QPushButton(tr("Dual"), body);
    dualOnlyButton_->setObjectName(QStringLiteral("panelModeButton"));
    dualOnlyButton_->setCheckable(true);
    dualOnlyButton_->setCursor(Qt::PointingHandCursor);
    dualOnlyButton_->setToolTip(
        tr("Data on left, Properties on right, console embedded below the plot (not full width)"));

    dualReversedOnlyButton_ = new QPushButton(tr("Reversed"), body);
    dualReversedOnlyButton_->setObjectName(QStringLiteral("panelModeButton"));
    dualReversedOnlyButton_->setCheckable(true);
    dualReversedOnlyButton_->setCursor(Qt::PointingHandCursor);
    dualReversedOnlyButton_->setToolTip(
        tr("Properties on left, Data on right, console embedded below the plot (not full width)"));

    dualWithConsoleButton_ = new QPushButton(tr("Dual + Console"), body);
    dualWithConsoleButton_->setObjectName(QStringLiteral("panelModeButton"));
    dualWithConsoleButton_->setCheckable(true);
    dualWithConsoleButton_->setCursor(Qt::PointingHandCursor);
    dualWithConsoleButton_->setToolTip(
        tr("Data on left, Properties on right, full-width console at bottom (default)"));

    dualReversedWithConsoleButton_ = new QPushButton(tr("Reversed + Console"), body);
    dualReversedWithConsoleButton_->setObjectName(QStringLiteral("panelModeButton"));
    dualReversedWithConsoleButton_->setCheckable(true);
    dualReversedWithConsoleButton_->setCursor(Qt::PointingHandCursor);
    dualReversedWithConsoleButton_->setToolTip(
        tr("Properties on left, Data on right, full-width console at bottom"));

    modeGroup_->addButton(dualOnlyButton_, 0);
    modeGroup_->addButton(dualWithConsoleButton_, 1);
    modeGroup_->addButton(dualReversedOnlyButton_, 2);
    modeGroup_->addButton(dualReversedWithConsoleButton_, 3);

    // 排列（用户示意图 2026-08-31）：上排 = 非全尺寸 Console（Dual / Reversed），
    // 下排 = 全尺寸 Console（Dual + Console / Reversed + Console）
    modeGrid->addWidget(dualOnlyButton_, 0, 0);
    modeGrid->addWidget(dualReversedOnlyButton_, 0, 1);
    modeGrid->addWidget(dualWithConsoleButton_, 1, 0);
    modeGrid->addWidget(dualReversedWithConsoleButton_, 1, 1);
    bodyLayout->addLayout(modeGrid);

    // ---- 显隐区（FR-004：三面板独立显隐；expand 由 applyPanelLayout 处理）----
    bodyLayout->addWidget(new QLabel(tr("Panel Visibility"), body));
    dataCheck_ = new QCheckBox(tr("Data"), body);
    dataCheck_->setObjectName(QStringLiteral("panelDataVisibleCheck"));
    propertyCheck_ = new QCheckBox(tr("Properties"), body);
    propertyCheck_->setObjectName(QStringLiteral("panelPropertyVisibleCheck"));
    consoleCheck_ = new QCheckBox(tr("Python Console"), body);
    consoleCheck_->setObjectName(QStringLiteral("panelConsoleVisibleCheck"));
    bodyLayout->addWidget(dataCheck_);
    bodyLayout->addWidget(propertyCheck_);
    bodyLayout->addWidget(consoleCheck_);

    // ---- 实时示意图（FR-006：几何复用 PanelLayoutConfig，与真实排布一致）----
    preview_ = new PanelPreviewWidget(body);
    bodyLayout->addWidget(preview_);

    // ---- 底部按钮条（恢复默认 / OK / Cancel）----
    auto* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    buttonRow->addStretch();
    restoreButton_ = new QPushButton(tr("Defaults"), body);
    restoreButton_->setObjectName(QStringLiteral("panelResetDefaultsButton"));
    restoreButton_->setCursor(Qt::PointingHandCursor);
    restoreButton_->setToolTip(tr("Restore default layout: Dual + Console, all panels visible"));
    okButton_ = new QPushButton(tr("OK"), body);
    okButton_->setObjectName(QStringLiteral("panelOkButton"));
    okButton_->setCursor(Qt::PointingHandCursor);
    cancelButton_ = new QPushButton(tr("Cancel"), body);
    cancelButton_->setObjectName(QStringLiteral("panelCancelButton"));
    cancelButton_->setCursor(Qt::PointingHandCursor);
    buttonRow->addWidget(restoreButton_);
    buttonRow->addWidget(okButton_);
    buttonRow->addWidget(cancelButton_);
    bodyLayout->addLayout(buttonRow);

    root->addWidget(body);

    // ---- 交互接线（任一变更 → 发射 configChanged，US3 实时预览）----
    // 模式按钮：QButtonGroup::idClicked 携带点击按钮 id（Qt 5.15），emitConfigForMode
    // 据此合成配置——不依赖 checkedId 更新时序（clicked 先于 setChecked 发射）。
    connect(modeGroup_, &QButtonGroup::idClicked, this,
            &PanelSettingsDialog::emitConfigForMode);
    connect(dataCheck_, &QCheckBox::toggled, this, &PanelSettingsDialog::emitConfig);
    connect(propertyCheck_, &QCheckBox::toggled, this, &PanelSettingsDialog::emitConfig);
    connect(consoleCheck_, &QCheckBox::toggled, this, &PanelSettingsDialog::emitConfig);
    connect(restoreButton_, &QPushButton::clicked, this,
            &PanelSettingsDialog::restoreDefaults);
    connect(cancelButton_, &QPushButton::clicked, this, &PanelSettingsDialog::reject);
    connect(okButton_, &QPushButton::clicked, this, &PanelSettingsDialog::accept);

    // 打开时回显初始配置（US1：不触发重放，主窗口已是该配置）
    syncFrom(initial);
}

PanelLayoutConfig PanelSettingsDialog::current() const {
    PanelLayoutConfig cfg;
    switch (modeGroup_ ? modeGroup_->checkedId() : -1) {
    case 0:
        cfg.mode = PanelLayoutMode::DualOnly;
        break;
    case 2:
        cfg.mode = PanelLayoutMode::DualReversedOnly;
        break;
    case 3:
        cfg.mode = PanelLayoutMode::DualReversedWithConsole;
        break;
    case 1:
    default:
        cfg.mode = PanelLayoutMode::DualWithConsole;
        break;
    }
    cfg.dataVisible = dataCheck_ ? dataCheck_->isChecked() : true;
    cfg.propertyVisible = propertyCheck_ ? propertyCheck_->isChecked() : true;
    cfg.consoleVisible = consoleCheck_ ? consoleCheck_->isChecked() : true;
    return cfg;
}

void PanelSettingsDialog::emitConfig() {
    const PanelLayoutConfig cfg = current();
    // 去重：exclusive 模式组一次切换可能触发多次（toggled true/false 双触发），
    // 相同最终状态只发射一次（一次变更一次信号）
    if (firstEmitDone_ && cfg == lastEmittedConfig_) return;
    firstEmitDone_ = true;
    lastEmittedConfig_ = cfg;
    if (preview_) preview_->setConfig(cfg);
    emit configChanged(cfg);
}

void PanelSettingsDialog::emitConfigForMode(int id) {
    PanelLayoutMode mode;
    switch (id) {
    case 0:
        mode = PanelLayoutMode::DualOnly;
        break;
    case 2:
        mode = PanelLayoutMode::DualReversedOnly;
        break;
    case 3:
        mode = PanelLayoutMode::DualReversedWithConsole;
        break;
    case 1:
    default:
        mode = PanelLayoutMode::DualWithConsole;
        break;
    }
    // 模式预设决定 PyShell 默认显隐（四种模式均含 PyShell，仅尺寸形态不同：
    // *Only 非全尺寸嵌入 / *WithConsole 全尺寸全宽）
    {
        const QSignalBlocker blocker(consoleCheck_);
        consoleCheck_->setChecked(modeDefaultsConsoleVisible(mode));
    }
    // 控件已刷新，按 current() 合成并发射完整配置
    emitConfig();
}

void PanelSettingsDialog::restoreDefaults() {
    // 恢复默认（FR-002 默认 = DualWithConsole + 三面板全显）并立即预览 + 发射
    syncFrom(PanelLayoutConfig());
    emitConfig();
}

void PanelSettingsDialog::syncFrom(const PanelLayoutConfig& cfg) {
    // 程序性回显不触发 configChanged（打开回显 / Cancel 回滚由调用方显式重放），
    // 故屏蔽控件 toggled 信号（与用户交互区分，QSignalBlocker 见 dialog_title_bar 约定）
    {
        const QSignalBlocker b1(dualOnlyButton_);
        const QSignalBlocker b2(dualWithConsoleButton_);
        const QSignalBlocker b3(dualReversedOnlyButton_);
        const QSignalBlocker b4(dualReversedWithConsoleButton_);
        const QSignalBlocker b5(dataCheck_);
        const QSignalBlocker b6(propertyCheck_);
        const QSignalBlocker b7(consoleCheck_);
        switch (cfg.mode) {
        case PanelLayoutMode::DualOnly:
            dualOnlyButton_->setChecked(true);
            break;
        case PanelLayoutMode::DualReversedOnly:
            dualReversedOnlyButton_->setChecked(true);
            break;
        case PanelLayoutMode::DualReversedWithConsole:
            dualReversedWithConsoleButton_->setChecked(true);
            break;
        case PanelLayoutMode::DualWithConsole:
        default:
            dualWithConsoleButton_->setChecked(true);
            break;
        }
        dataCheck_->setChecked(cfg.dataVisible);
        propertyCheck_->setChecked(cfg.propertyVisible);
        consoleCheck_->setChecked(cfg.consoleVisible);
    }
    if (preview_) preview_->setConfig(cfg);
}

void PanelSettingsDialog::accept() {
    // OK 语义（US1 场景 3）：持久化四 key（FR-007，data-model §5）；
    // 当前配置已实时生效，无需重放
    const PanelLayoutConfig cfg = current();
    QSettings s;
    s.setValue(QLatin1String(kPanelSettingsModeKey), modeToKey(cfg.mode));
    s.setValue(QLatin1String(kPanelSettingsDataKey), cfg.dataVisible);
    s.setValue(QLatin1String(kPanelSettingsPropertyKey), cfg.propertyVisible);
    s.setValue(QLatin1String(kPanelSettingsConsoleKey), cfg.consoleVisible);
    QDialog::accept();
}

void PanelSettingsDialog::reject() {
    // Cancel / 标题栏关闭语义（US3 场景 3）：恢复打开前快照并重放，
    // 主窗口回滚到打开前布局；关闭后下次打开以新快照为基准
    syncFrom(cfg0_);
    emit configChanged(cfg0_);
    QDialog::reject();
}

void PanelSettingsDialog::mousePressEvent(QMouseEvent* event) {
    // 标题栏拖拽移动（去系统标题栏后自行处理；与 layout_settings_dialog 等弹窗一致）
    if (event->button() == Qt::LeftButton && titleBar_ &&
        event->pos().y() <= titleBar_->height()) {
        dragOffset_ = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void PanelSettingsDialog::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
        move(event->globalPos() - dragOffset_);
    }
    QDialog::mouseMoveEvent(event);
}

void PanelSettingsDialog::mouseReleaseEvent(QMouseEvent* event) {
    dragOffset_ = QPoint();
    QDialog::mouseReleaseEvent(event);
}

}  // namespace ui
}  // namespace perception

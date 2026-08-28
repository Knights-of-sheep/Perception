#include "ui/subwindow/layout_settings_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

namespace perception {
namespace ui {

namespace {
// QSpinBox 范围：0 = 未设置（自动/不限），1–10 为有效上限（FR-007/008）
constexpr int kMaxConstraintLimit = 10;
}  // namespace

LayoutSettingsDialog::LayoutSettingsDialog(const LayoutConfig& initial, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Layout Settings"));
    setObjectName(QStringLiteral("layoutSettingsDialog"));

    auto* form = new QGridLayout(this);
    form->setContentsMargins(12, 12, 12, 12);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);

    // 排列模式（FR-004~006）
    form->addWidget(new QLabel(tr("Arrange:"), this), 0, 0);
    modeCombo_ = new QComboBox(this);
    modeCombo_->setObjectName(QStringLiteral("layoutModeCombo"));
    modeCombo_->addItem(tr("Grid"), static_cast<int>(LayoutMode::Grid));
    modeCombo_->addItem(tr("By Row"), static_cast<int>(LayoutMode::Row));
    modeCombo_->addItem(tr("By Column"), static_cast<int>(LayoutMode::Column));
    form->addWidget(modeCombo_, 0, 1);

    // 最大列数 / 最大行数（0 = 未设置；按模式显隐，见 updateConstraintVisibility）
    maxColsLabel_ = new QLabel(tr("Max columns:"), this);
    form->addWidget(maxColsLabel_, 1, 0);
    maxColsSpin_ = new QSpinBox(this);
    maxColsSpin_->setObjectName(QStringLiteral("layoutMaxColsSpin"));
    maxColsSpin_->setRange(0, kMaxConstraintLimit);
    maxColsSpin_->setSpecialValueText(tr("Auto (fit width)"));
    form->addWidget(maxColsSpin_, 1, 1);

    maxRowsLabel_ = new QLabel(tr("Max rows:"), this);
    form->addWidget(maxRowsLabel_, 2, 0);
    maxRowsSpin_ = new QSpinBox(this);
    maxRowsSpin_->setObjectName(QStringLiteral("layoutMaxRowsSpin"));
    maxRowsSpin_->setRange(0, kMaxConstraintLimit);
    maxRowsSpin_->setSpecialValueText(tr("Unlimited"));
    form->addWidget(maxRowsSpin_, 2, 1);

    // 保持相同宽高（FR-009）
    sameSizeCheck_ = new QCheckBox(tr("Keep same size"), this);
    sameSizeCheck_->setObjectName(QStringLiteral("layoutSameSizeCheck"));
    form->addWidget(sameSizeCheck_, 3, 0, 1, 2);

    // 提示（QSpinBox 范围天然拦截非法输入，FR-012）
    auto* hint = new QLabel(tr("Values 1–10; 0 means automatic."), this);
    hint->setObjectName(QStringLiteral("layoutHintLabel"));
    hint->setWordWrap(true);
    form->addWidget(hint, 4, 0, 1, 2);

    // 修改即时生效（FR-011：无需确认按钮）
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { updateConstraintVisibility(); });
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(maxColsSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(maxRowsSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(sameSizeCheck_, &QCheckBox::toggled, this, &LayoutSettingsDialog::emitConfig);

    syncFrom(initial);  // 打开时回显当前配置（US5）
    updateConstraintVisibility();
}

void LayoutSettingsDialog::syncFrom(const LayoutConfig& cfg) {
    const int modeIndex = modeCombo_->findData(static_cast<int>(cfg.mode));
    if (modeIndex >= 0) {
        QSignalBlocker blocker(modeCombo_);
        modeCombo_->setCurrentIndex(modeIndex);
    }
    {
        QSignalBlocker blocker(maxColsSpin_);
        maxColsSpin_->setValue(cfg.maxCols);
    }
    {
        QSignalBlocker blocker(maxRowsSpin_);
        maxRowsSpin_->setValue(cfg.maxRows);
    }
    {
        QSignalBlocker blocker(sameSizeCheck_);
        sameSizeCheck_->setChecked(cfg.sameSize);
    }
}

LayoutConfig LayoutSettingsDialog::current() const {
    LayoutConfig cfg;
    cfg.mode = static_cast<LayoutMode>(modeCombo_->currentData().toInt());
    cfg.maxCols = maxColsSpin_->value();
    cfg.maxRows = maxRowsSpin_->value();
    cfg.sameSize = sameSizeCheck_->isChecked();
    return cfg;
}

void LayoutSettingsDialog::emitConfig() {
    emit configChanged(current());
}

void LayoutSettingsDialog::updateConstraintVisibility() {
    const auto mode = static_cast<LayoutMode>(modeCombo_->currentData().toInt());
    // 优先列排（By Column）：仅显示 最大列数；优先行排（By Row）：仅显示 最大行数；
    // Grid：两个都显示（FR-007/008）。
    const bool showCols = (mode == LayoutMode::Grid || mode == LayoutMode::Column);
    const bool showRows = (mode == LayoutMode::Grid || mode == LayoutMode::Row);
    if (maxColsLabel_) maxColsLabel_->setVisible(showCols);
    if (maxColsSpin_) maxColsSpin_->setVisible(showCols);
    if (maxRowsLabel_) maxRowsLabel_->setVisible(showRows);
    if (maxRowsSpin_) maxRowsSpin_->setVisible(showRows);
}

}  // namespace ui
}  // namespace perception

#include "ui/subwindow/layout_settings_dialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "ui/dialog_title_bar.h"

namespace perception {
namespace ui {

namespace {
// QSpinBox 范围：0 = 未设置（自动/不限），1–10 为有效上限（FR-007/008）
constexpr int kMaxConstraintLimit = 10;
}  // namespace

LayoutSettingsDialog::LayoutSettingsDialog(const LayoutConfig& initial, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
    setWindowTitle(tr("Layout Settings"));
    setWindowIcon(QApplication::windowIcon());
    setObjectName(QStringLiteral("layoutSettingsDialog"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 无边框自定义标题栏：与其他弹窗（帮助/关于/文件对话框）一致
    //（图标+标题+关闭按钮，可拖拽移动；FR-011 / 宪法「技术栈约束 · GUI」）
    titleBar_ = buildDialogTitleBar(this, tr("Layout Settings"));
    root->addWidget(titleBar_);

    auto* form = new QGridLayout();
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

    // 优先行 / 优先列（仅 Grid 模式显示；2026-08-29 用户反馈：
    // Grid 布局支持设置填充方向，无约束时决定比例网格的行/列优先）
    priorityLabel_ = new QLabel(tr("Priority:"), this);
    form->addWidget(priorityLabel_, 3, 0);
    priorityRowRadio_ = new QRadioButton(tr("By row"), this);
    priorityRowRadio_->setObjectName(QStringLiteral("layoutPriorityRowRadio"));
    priorityColRadio_ = new QRadioButton(tr("By column"), this);
    priorityColRadio_->setObjectName(QStringLiteral("layoutPriorityColRadio"));
    auto* priorityRow = new QHBoxLayout();
    priorityRow->setSpacing(12);
    priorityRow->addWidget(priorityRowRadio_);
    priorityRow->addWidget(priorityColRadio_);
    priorityRow->addStretch();
    form->addLayout(priorityRow, 3, 1);

    // 保持相同宽高（FR-009）
    sameSizeCheck_ = new QCheckBox(tr("Keep same size"), this);
    sameSizeCheck_->setObjectName(QStringLiteral("layoutSameSizeCheck"));
    form->addWidget(sameSizeCheck_, 4, 0, 1, 2);

    // 提示（QSpinBox 范围天然拦截非法输入，FR-012）
    auto* hint = new QLabel(tr("Values 1–10; 0 means automatic."), this);
    hint->setObjectName(QStringLiteral("layoutHintLabel"));
    hint->setWordWrap(true);
    form->addWidget(hint, 5, 0, 1, 2);

    root->addLayout(form);

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
    connect(priorityRowRadio_, &QRadioButton::toggled, this,
            [this](bool) { emitConfig(); });
    connect(priorityColRadio_, &QRadioButton::toggled, this,
            [this](bool) { emitConfig(); });

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
    {
        QSignalBlocker blocker(priorityRowRadio_);
        priorityRowRadio_->setChecked(cfg.gridDirection == GridDirection::Row);
    }
    {
        QSignalBlocker blocker(priorityColRadio_);
        priorityColRadio_->setChecked(cfg.gridDirection == GridDirection::Column);
    }
}

LayoutConfig LayoutSettingsDialog::current() const {
    LayoutConfig cfg;
    cfg.mode = static_cast<LayoutMode>(modeCombo_->currentData().toInt());
    cfg.maxCols = maxColsSpin_->value();
    cfg.maxRows = maxRowsSpin_->value();
    cfg.sameSize = sameSizeCheck_->isChecked();
    cfg.gridDirection =
        priorityColRadio_->isChecked() ? GridDirection::Column : GridDirection::Row;
    return cfg;
}

void LayoutSettingsDialog::emitConfig() {
    emit configChanged(current());
}

void LayoutSettingsDialog::updateConstraintVisibility() {
    const auto mode = static_cast<LayoutMode>(modeCombo_->currentData().toInt());
    // 优先列排（By Column）：仅显示 最大列数；优先行排（By Row）：仅显示 最大行数；
    // Grid：显示 最大列数 + 最大行数 + 优先行/优先列（FR-007/008 + 2026-08-29 反馈）。
    const bool showCols = (mode == LayoutMode::Grid || mode == LayoutMode::Column);
    const bool showRows = (mode == LayoutMode::Grid || mode == LayoutMode::Row);
    const bool showPriority = (mode == LayoutMode::Grid);
    if (maxColsLabel_) maxColsLabel_->setVisible(showCols);
    if (maxColsSpin_) maxColsSpin_->setVisible(showCols);
    if (maxRowsLabel_) maxRowsLabel_->setVisible(showRows);
    if (maxRowsSpin_) maxRowsSpin_->setVisible(showRows);
    if (priorityLabel_) priorityLabel_->setVisible(showPriority);
    if (priorityRowRadio_) priorityRowRadio_->setVisible(showPriority);
    if (priorityColRadio_) priorityColRadio_->setVisible(showPriority);
}

void LayoutSettingsDialog::mousePressEvent(QMouseEvent* event) {
    // 标题栏拖拽移动（去系统标题栏后自行处理；与其他弹窗一致，FR-011）
    if (event->button() == Qt::LeftButton && event->pos().y() <= titleBar_->height()) {
        dragOffset_ = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void LayoutSettingsDialog::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
        move(event->globalPos() - dragOffset_);
    }
    QDialog::mouseMoveEvent(event);
}

void LayoutSettingsDialog::mouseReleaseEvent(QMouseEvent* event) {
    dragOffset_ = QPoint();
    QDialog::mouseReleaseEvent(event);
}

}  // namespace ui
}  // namespace perception

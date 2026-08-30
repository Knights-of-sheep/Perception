#include "ui/subwindow/layout_settings_dialog.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "ui/dialog_title_bar.h"
#include "ui/subwindow/layout_preview_widget.h"

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
    // 列 1 控件列占满弹性空间、列 0 label 列自适应内容（避免 "By column" 文字被挤压截断）
    form->setColumnStretch(0, 0);
    form->setColumnStretch(1, 1);

    // 排列模式：分段按钮组（FR-004~006；008 重构：QComboBox → 可勾选分段按钮，点击即应用）
    form->addWidget(new QLabel(tr("Arrange:"), this), 0, 0);
    modeGroup_ = new QButtonGroup(this);
    modeGroup_->setExclusive(true);
    gridButton_ = new QPushButton(tr("Grid"), this);
    rowButton_ = new QPushButton(tr("By Row"), this);
    colButton_ = new QPushButton(tr("By Column"), this);
    auto* modeRow = new QHBoxLayout();
    modeRow->setSpacing(4);
    for (auto* b : {gridButton_, rowButton_, colButton_}) {
        b->setObjectName(QStringLiteral("layoutModeButton"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        modeGroup_->addButton(b);
        modeRow->addWidget(b);
        // 点击切换即应用（exclusive 组内 checked 已先切换），显隐按新模式更新
        connect(b, &QPushButton::clicked, this, [this] {
            updateConstraintVisibility();
            emitConfig();
        });
    }
    modeRow->addStretch();
    form->addLayout(modeRow, 0, 1);

    // 优先级（FR-010 约束轴 = 生效轴，行优先 → 最大列数、列优先 → 最大行数）
    // 控件始终可见，按 mode == Grid 启用/灰显——避免模式切换造成弹窗布局跳动
    priorityLabel_ = new QLabel(tr("Priority:"), this);
    form->addWidget(priorityLabel_, 1, 0);
    priorityRowRadio_ = new QRadioButton(tr("By row"), this);
    priorityRowRadio_->setObjectName(QStringLiteral("layoutPriorityRowRadio"));
    priorityColRadio_ = new QRadioButton(tr("By column"), this);
    priorityColRadio_->setObjectName(QStringLiteral("layoutPriorityColRadio"));
    auto* priorityRow = new QHBoxLayout();
    priorityRow->setSpacing(8);
    // HBoxLayout 按 minimum size 计算——确保 "By column" 文字不被挤压截断
    priorityRow->setSizeConstraint(QLayout::SetMinimumSize);
    priorityRow->addWidget(priorityRowRadio_);
    priorityRow->addWidget(priorityColRadio_);
    priorityRow->addStretch();
    form->addLayout(priorityRow, 1, 1);

    // 最大行数（仅 Grid + 列优先 生效，0 = 未设置，FR-008）—— 始终可见，未生效时灰显
    maxRowsLabel_ = new QLabel(tr("Max rows:"), this);
    form->addWidget(maxRowsLabel_, 2, 0);
    maxRowsSpin_ = new QSpinBox(this);
    maxRowsSpin_->setObjectName(QStringLiteral("layoutMaxRowsSpin"));
    maxRowsSpin_->setRange(0, kMaxConstraintLimit);
    maxRowsSpin_->setSpecialValueText(tr("Unlimited"));
    form->addWidget(maxRowsSpin_, 2, 1);

    // 最大列数（仅 Grid + 行优先 生效，0 = 未设置，FR-007）—— 始终可见，未生效时灰显
    maxColsLabel_ = new QLabel(tr("Max columns:"), this);
    form->addWidget(maxColsLabel_, 3, 0);
    maxColsSpin_ = new QSpinBox(this);
    maxColsSpin_->setObjectName(QStringLiteral("layoutMaxColsSpin"));
    maxColsSpin_->setRange(0, kMaxConstraintLimit);
    maxColsSpin_->setSpecialValueText(tr("Unlimited"));
    form->addWidget(maxColsSpin_, 3, 1);

    // 间隙宽度（FR-015 / 008：0–50 px，默认 6）
    gapLabel_ = new QLabel(tr("Gap:"), this);
    form->addWidget(gapLabel_, 4, 0);
    gapSpin_ = new QSpinBox(this);
    gapSpin_->setObjectName(QStringLiteral("layoutGapSpin"));
    gapSpin_->setRange(0, 50);
    gapSpin_->setSuffix(tr(" px"));
    form->addWidget(gapSpin_, 4, 1);

    // 保持相同宽高（FR-009）
    sameSizeCheck_ = new QCheckBox(tr("Keep same size"), this);
    sameSizeCheck_->setObjectName(QStringLiteral("layoutSameSizeCheck"));
    form->addWidget(sameSizeCheck_, 5, 0, 1, 2);

    // 实时排列预览（FR-013：几何复用 LayoutManager 已测纯函数，与真实排布必然一致）
    form->addWidget(new QLabel(tr("Preview:"), this), 6, 0);
    preview_ = new LayoutPreviewWidget(this);
    form->addWidget(preview_, 6, 1);

    // 恢复默认（FR-014）：Grid + 行优先 + 无约束 + 不保持相同宽高 + 间隙 6
    restoreButton_ = new QPushButton(tr("Restore Defaults"), this);
    restoreButton_->setObjectName(QStringLiteral("layoutRestoreButton"));
    form->addWidget(restoreButton_, 7, 0, 1, 2, Qt::AlignRight);

    root->addLayout(form);

    // 修改即时生效（FR-011：无需确认按钮）
    connect(maxRowsSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(maxColsSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(sameSizeCheck_, &QCheckBox::toggled, this, &LayoutSettingsDialog::emitConfig);
    // 切换优先级 radio 时同步刷新显隐：行优先 → 显示最大列数、列优先 → 显示最大行数（FR-010）
    connect(priorityRowRadio_, &QRadioButton::toggled, this, [this](bool) {
        updateConstraintVisibility();
        emitConfig();
    });
    connect(priorityColRadio_, &QRadioButton::toggled, this, [this](bool) {
        updateConstraintVisibility();
        emitConfig();
    });
    connect(gapSpin_, qOverload<int>(&QSpinBox::valueChanged), this,
            &LayoutSettingsDialog::emitConfig);
    connect(restoreButton_, &QPushButton::clicked, this, &LayoutSettingsDialog::restoreDefaults);

    syncFrom(initial);  // 打开时回显当前配置（US5）
    updateConstraintVisibility();
    preview_->setConfig(current());
    // 弹窗最小宽度：保证 Priority 行 "By column" 文字、Max columns: label 与 preview 完整显示
    setMinimumWidth(400);
}

void LayoutSettingsDialog::syncFrom(const LayoutConfig& cfg) {
    {
        QSignalBlocker blocker(modeGroup_);
        gridButton_->setChecked(cfg.mode == LayoutMode::Grid);
        rowButton_->setChecked(cfg.mode == LayoutMode::Row);
        colButton_->setChecked(cfg.mode == LayoutMode::Column);
    }
    {
        QSignalBlocker blocker(maxRowsSpin_);
        maxRowsSpin_->setValue(cfg.maxRows);
    }
    {
        QSignalBlocker blocker(maxColsSpin_);
        maxColsSpin_->setValue(cfg.maxCols);
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
    {
        QSignalBlocker blocker(gapSpin_);
        gapSpin_->setValue(cfg.spacing);
    }
}

LayoutConfig LayoutSettingsDialog::current() const {
    LayoutConfig cfg;
    if (gridButton_->isChecked()) {
        cfg.mode = LayoutMode::Grid;
    } else if (rowButton_->isChecked()) {
        cfg.mode = LayoutMode::Row;
    } else if (colButton_->isChecked()) {
        cfg.mode = LayoutMode::Column;
    }
    cfg.maxRows = maxRowsSpin_->value();
    cfg.maxCols = maxColsSpin_->value();
    cfg.sameSize = sameSizeCheck_->isChecked();
    cfg.spacing = gapSpin_->value();
    cfg.gridDirection =
        priorityColRadio_->isChecked() ? GridDirection::Column : GridDirection::Row;
    return cfg;
}

void LayoutSettingsDialog::emitConfig() {
    const LayoutConfig cfg = current();
    if (preview_) preview_->setConfig(cfg);  // 实时预览与真实排布同源（契约 §6）
    emit configChanged(cfg);
}

void LayoutSettingsDialog::updateConstraintVisibility() {
    // 控件可用性矩阵（FR-010 / 008 契约 §2/§3）：
    //   所有控件始终可见，禁用项灰显——保持弹窗几何稳定，模式/优先级切换不造成布局跳动（SC-006）。
    //   约束轴单一判定源：LayoutManager::constraintAxis。
    //   Grid + 行优先 → maxCols 生效，maxRows 灰显；
    //   Grid + 列优先 → maxRows 生效，maxCols 灰显；
    //   By Row / By Column → 优先级 + 最大行 + 最大列 全部灰显（约束不参与计算）。
    const LayoutConfig cfg = current();
    const ConstraintAxis axis = LayoutManager().constraintAxis(cfg);
    const bool grid = (cfg.mode == LayoutMode::Grid);
    const bool enablePriority = grid;
    const bool enableMaxRows = grid && axis == ConstraintAxis::Row;
    const bool enableMaxCols = grid && axis == ConstraintAxis::Column;
    if (priorityLabel_) priorityLabel_->setEnabled(enablePriority);
    if (priorityRowRadio_) priorityRowRadio_->setEnabled(enablePriority);
    if (priorityColRadio_) priorityColRadio_->setEnabled(enablePriority);
    if (maxRowsLabel_) maxRowsLabel_->setEnabled(enableMaxRows);
    if (maxRowsSpin_) maxRowsSpin_->setEnabled(enableMaxRows);
    if (maxColsLabel_) maxColsLabel_->setEnabled(enableMaxCols);
    if (maxColsSpin_) maxColsSpin_->setEnabled(enableMaxCols);
}

void LayoutSettingsDialog::restoreDefaults() {
    // 默认配置（LayoutConfig 默认值）：Grid + 行优先 + 无约束 + 不保持相同宽高 + 间隙 6
    const LayoutConfig d;
    {
        QSignalBlocker blocker(modeGroup_);
        gridButton_->setChecked(true);
        rowButton_->setChecked(false);
        colButton_->setChecked(false);
    }
    {
        QSignalBlocker blocker(priorityRowRadio_);
        priorityRowRadio_->setChecked(true);
    }
    {
        QSignalBlocker blocker(priorityColRadio_);
        priorityColRadio_->setChecked(false);
    }
    {
        QSignalBlocker blocker(maxRowsSpin_);
        maxRowsSpin_->setValue(0);
    }
    {
        QSignalBlocker blocker(maxColsSpin_);
        maxColsSpin_->setValue(0);
    }
    {
        QSignalBlocker blocker(sameSizeCheck_);
        sameSizeCheck_->setChecked(false);
    }
    {
        QSignalBlocker blocker(gapSpin_);
        gapSpin_->setValue(d.spacing);
    }
    updateConstraintVisibility();
    emitConfig();
}

void LayoutSettingsDialog::setPreviewCount(int count) {
    if (preview_) preview_->setPreviewCount(count);
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

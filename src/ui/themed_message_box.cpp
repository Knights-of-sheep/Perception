// ===== ThemedMessageBox 实现 =====
// 008-unify-dialog-styling：统一无边框消息框。
// 布局：共享标题栏（buildDialogTitleBar，SC-003）+ 图标/正文 + 按钮行；
// 背景继承 QDialog 的 @dialogBg@（QSS），拖拽/Esc 语义对齐 FramelessDialog。
#include "ui/themed_message_box.h"

#include "ui/dialog_title_bar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace perception {
namespace ui {

namespace {
// 标准按钮文本（Qt QMessageBox 同款映射，版本无关实现）
QString standardButtonText(QMessageBox::StandardButton b) {
    switch (b) {
        case QMessageBox::Ok:              return QObject::tr("OK");
        case QMessageBox::Save:            return QObject::tr("Save");
        case QMessageBox::SaveAll:         return QObject::tr("Save All");
        case QMessageBox::Open:            return QObject::tr("Open");
        case QMessageBox::Yes:             return QObject::tr("Yes");
        case QMessageBox::YesToAll:        return QObject::tr("Yes to All");
        case QMessageBox::No:              return QObject::tr("No");
        case QMessageBox::NoToAll:         return QObject::tr("No to All");
        case QMessageBox::Abort:           return QObject::tr("Abort");
        case QMessageBox::Retry:           return QObject::tr("Retry");
        case QMessageBox::Ignore:          return QObject::tr("Ignore");
        case QMessageBox::Close:           return QObject::tr("Close");
        case QMessageBox::Cancel:          return QObject::tr("Cancel");
        case QMessageBox::Discard:         return QObject::tr("Discard");
        case QMessageBox::Help:            return QObject::tr("Help");
        case QMessageBox::Apply:           return QObject::tr("Apply");
        case QMessageBox::Reset:           return QObject::tr("Reset");
        case QMessageBox::RestoreDefaults: return QObject::tr("Restore Defaults");
        default:                           return QObject::tr("OK");
    }
}

// 消息图标 → QStyle 标准图标（Qt QMessageBox 同款映射）
QStyle::StandardPixmap stylePixmapForIcon(QMessageBox::Icon icon) {
    switch (icon) {
        case QMessageBox::Information: return QStyle::SP_MessageBoxInformation;
        case QMessageBox::Warning:     return QStyle::SP_MessageBoxWarning;
        case QMessageBox::Critical:    return QStyle::SP_MessageBoxCritical;
        case QMessageBox::Question:    return QStyle::SP_MessageBoxQuestion;
        case QMessageBox::NoIcon:
        default:                       return QStyle::SP_CustomBase;  // 不显示图标
    }
}
}  // namespace

ThemedMessageBox::ThemedMessageBox(QWidget* parent, QMessageBox::Icon icon,
                                   const QString& title, const QString& text,
                                   QMessageBox::StandardButtons buttons,
                                   QMessageBox::StandardButton defaultButton)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint),  // 无边框（FR-004）
      clicked_(defaultButton) {
    setModal(true);
    setWindowTitle(title);
    setMinimumWidth(420);

    // 标题栏：共享工厂 → 外观与帮助/关于/文件对话框一致（SC-003）
    titleBar_ = buildDialogTitleBar(this, title);

    // 内容行：图标 + 正文
    auto* body = new QHBoxLayout();
    body->setContentsMargins(16, 14, 16, 6);
    body->setSpacing(12);
    if (icon != QMessageBox::NoIcon) {
        auto* iconLabel = new QLabel(this);
        iconLabel->setPixmap(style()->standardIcon(stylePixmapForIcon(icon)).pixmap(32, 32));
        iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        body->addWidget(iconLabel);
    }
    auto* textLabel = new QLabel(text, this);
    textLabel->setWordWrap(true);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    body->addWidget(textLabel, 1);

    // 按钮行：Qt 标准按钮文本映射；defaultButton 走 QSS :default 主按钮样式
    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(16, 8, 16, 14);
    btnRow->addStretch(1);
    int bits = static_cast<int>(buttons);
    while (bits) {
        const int low = bits & -bits;  // 取最低位 → 稳定顺序
        bits ^= low;
        const auto sb = static_cast<QMessageBox::StandardButton>(low);
        auto* btn = new QPushButton(standardButtonText(sb), this);
        if (sb == defaultButton) {
            btn->setDefault(true);  // QSS :default → accent 主按钮
        }
        connect(btn, &QPushButton::clicked, this, [this, sb] {
            clicked_ = sb;
            accept();
        });
        btnRow->addWidget(btn);
    }

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(titleBar_);
    root->addLayout(body);
    root->addLayout(btnRow);

    adjustSize();
}

// ---- 标题栏拖拽移动（同 FramelessDialog：点击标题栏空白处拖动）----
void ThemedMessageBox::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->pos().y() <= titleBar_->height()) {
        dragOffset_ = e->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(e);
}

void ThemedMessageBox::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
        move(e->globalPos() - dragOffset_);
    }
    QDialog::mouseMoveEvent(e);
}

void ThemedMessageBox::mouseReleaseEvent(QMouseEvent* e) {
    dragOffset_ = QPoint();
    QDialog::mouseReleaseEvent(e);
}

void ThemedMessageBox::keyPressEvent(QKeyEvent* event) {
    // Esc → defaultButton（clicked_ 构造时已初始化为 defaultButton，X 关闭同理）
    if (event->key() == Qt::Key_Escape) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

// ---- 工厂：栈对象 exec()，返回被点击按钮（与 QMessageBox 静态方法同语义）----
QMessageBox::StandardButton showThemedMessageBox(
    QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    ThemedMessageBox box(parent, icon, title, text, buttons, defaultButton);
    box.exec();
    return box.clickedButton();
}

}  // namespace ui
}  // namespace perception

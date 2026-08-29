// ===== FramelessDialog 实现 =====
#include "ui/frameless_dialog.h"

#include "ui/dialog_title_bar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace perception {
namespace ui {

FramelessDialog::FramelessDialog(QWidget* parent, const QString& title, const QString& html)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    setWindowTitle(title);
    setWindowIcon(QApplication::windowIcon());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 标题栏：复用主界面 titleBarRow 样式（@panelBg@ + 底部分隔线）
    titleBar_ = buildDialogTitleBar(this, title);
    root->addWidget(titleBar_);

    // 内容区：富文本正文 + 确定按钮（QSS 默认态 → primary accent）
    auto* body = new QVBoxLayout();
    body->setContentsMargins(20, 18, 20, 16);
    body->setSpacing(16);

    auto* content = new QLabel(this);
    content->setTextFormat(Qt::RichText);
    content->setWordWrap(true);
    content->setText(html);
    body->addWidget(content);

    auto* okRow = new QHBoxLayout();
    okRow->addStretch();
    auto* okBtn = new QPushButton(tr("OK"), this);
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::close);
    okRow->addWidget(okBtn);
    body->addLayout(okRow);

    root->addLayout(body);

    setMinimumWidth(400);
    adjustSize();
}

void FramelessDialog::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->pos().y() <= titleBar_->height()) {
        dragOffset_ = e->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(e);
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
        move(e->globalPos() - dragOffset_);
    }
    QDialog::mouseMoveEvent(e);
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* e) {
    dragOffset_ = QPoint();
    QDialog::mouseReleaseEvent(e);
}

void showFramelessDialog(QWidget* parent, const QString& title, const QString& html) {
    auto* dlg = new FramelessDialog(parent, title, html);
    dlg->exec();
}

}  // namespace ui
}  // namespace perception

#include "ui/themed_file_dialog.h"

#include "ui/dialog_title_bar.h"

#include <QApplication>
#include <QFileDialog>
#include <QIcon>  // QApplication::windowIcon() 返回完整类型
#include <QMouseEvent>
#include <QVBoxLayout>

namespace perception {
namespace ui {

ThemedFileDialog::ThemedFileDialog(QWidget* parent, const QString& title,
                                   const QString& dir, const QString& filter,
                                   FileDialogMode mode)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
    setWindowTitle(title);
    setWindowIcon(QApplication::windowIcon());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 标题栏：复用主界面 titleBarRow 样式
    titleBar_ = buildDialogTitleBar(this, title);
    root->addWidget(titleBar_);

    // QFileDialog 作为普通 widget 嵌入（Qt::Widget 剥除其窗口属性）
    fileDialog_ = new QFileDialog(this, title, dir, filter);
    fileDialog_->setOption(QFileDialog::DontUseNativeDialog, true);
    fileDialog_->setWindowFlags(Qt::Widget);
    fileDialog_->setSizeGripEnabled(false);
    if (mode == FileDialogMode::Directory) {
        fileDialog_->setFileMode(QFileDialog::Directory);
        fileDialog_->setOption(QFileDialog::ShowDirsOnly, true);
        fileDialog_->setOption(QFileDialog::DontResolveSymlinks, true);
    } else if (mode == FileDialogMode::Save) {
        fileDialog_->setAcceptMode(QFileDialog::AcceptSave);
        fileDialog_->setFileMode(QFileDialog::AnyFile);
    } else {
        fileDialog_->setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog_->setFileMode(QFileDialog::AnyFile);
    }
    connect(fileDialog_, &QFileDialog::accepted, this, &QDialog::accept);
    connect(fileDialog_, &QFileDialog::rejected, this, &QDialog::reject);

    root->addWidget(fileDialog_);

    setMinimumSize(640, 480);
    adjustSize();
}

QString ThemedFileDialog::selectedFile() const {
    return fileDialog_->selectedFiles().value(0);
}

void ThemedFileDialog::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->pos().y() <= titleBar_->height()) {
        dragOffset_ = e->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(e);
}

void ThemedFileDialog::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && !dragOffset_.isNull()) {
        move(e->globalPos() - dragOffset_);
    }
    QDialog::mouseMoveEvent(e);
}

void ThemedFileDialog::mouseReleaseEvent(QMouseEvent* e) {
    dragOffset_ = QPoint();
    QDialog::mouseReleaseEvent(e);
}

QString runThemedFileDialog(QWidget* parent, const QString& title,
                            const QString& dir, const QString& filter,
                            FileDialogMode mode) {
    ThemedFileDialog dlg(parent, title, dir, filter, mode);
    return (dlg.exec() == QDialog::Accepted) ? dlg.selectedFile() : QString();
}

}  // namespace ui
}  // namespace perception

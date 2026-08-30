#include "ui/themed_file_dialog.h"

#include "ui/dialog_title_bar.h"
#include "ui/theme/file_dialog_qss.h"
#include "ui/theme/theme_dialog_layer.h"
#include "ui/theme/theme_manager.h"
#include "ui/theme/theme_types.h"  // ThemeDescriptor / ThemeColors 完整定义

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

    // 008 跟进：Qt 5.15 QFileWidget 内部硬编码样式表屏蔽应用级 QSS（侧边栏/Look-in 行/
    // 底部按钮/视图停留在浅色原生观感）——按当前主题色板显式注入样式表恢复视觉统一。
    // 背景 dialogBg 走与全局 QSS 相同的派生路径（deriveDialogBg），保证弹窗层一致。
    if (const auto* cur = ThemeManager::current()) {
        const auto& c = cur->colors;
        const QColor dlgBg =
            c.dialogBg.isValid()
                ? c.dialogBg
                : theme::deriveDialogBg(c.windowBg, c.text, cur->family);
        fileDialog_->setStyleSheet(buildFileDialogQss(c, dlgBg));
    }

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

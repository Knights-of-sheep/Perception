// ===== FramelessDialog：无边框 + 自定义标题栏的通用信息弹窗 =====
// 背景：QMessageBox 是系统原生标题栏，与主界面/Dock 自定义标题栏风格割裂。
// 方案：Qt::FramelessWindowHint 的 QDialog，标题栏复用 buildDialogTitleBar，
//       正文富文本 + 确定按钮。
// 006-constitution-refactor：自 MainWindow.cpp 提取（帮助/关于弹窗通用入口）。
#pragma once

#include <QDialog>

class QMouseEvent;

namespace perception {
namespace ui {

// 无边框信息弹窗：自定义标题栏（buildDialogTitleBar）+ 富文本正文 + OK
class FramelessDialog : public QDialog {
public:
    FramelessDialog(QWidget* parent, const QString& title, const QString& html);

protected:
    // 标题栏拖拽移动（去系统标题栏后自行处理）
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QWidget* titleBar_ = nullptr;
    QPoint dragOffset_;
};

// 帮助/关于统一入口：模态显示无边框对话框
void showFramelessDialog(QWidget* parent, const QString& title, const QString& html);

}  // namespace ui
}  // namespace perception

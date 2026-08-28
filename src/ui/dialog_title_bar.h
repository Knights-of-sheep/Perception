// ===== 通用对话框标题栏工厂（弹窗统一风格，宪法「技术栈约束 · GUI」）=====
// 所有应用弹窗（布局设置/帮助/关于/文件对话框）共享同一无边框标题栏：
// 复用 QSS objectName (titleBarRow/titleBarTitle/titleBarIcon/winCloseBtn) 与
// makeWinBtnIcon 图标，保证所有无边框弹窗标题栏外观一致、随主题、可拖拽移动。
// owner: 标题栏所有者（QDialog 子类），close 按钮连接其 close()。
#pragma once

#include <QString>

class QDialog;
class QWidget;

namespace perception {
namespace ui {

QWidget* buildDialogTitleBar(QDialog* owner, const QString& title);

}  // namespace ui
}  // namespace perception

// ===== ThemedMessageBox：统一无边框消息框（008-unify-dialog-styling）=====
// 消除 QMessageBox 系统原生标题栏例外（FR-004/FR-005）：
//   - Qt::FramelessWindowHint 无边框，复用 buildDialogTitleBar 共享标题栏（SC-003）
//   - 背景随 QSS @dialogBg@ 自动生效（弹窗背景层，派生见 theme_dialog_layer.h）
//   - 模态 exec()；关闭按钮 / Esc 返回 defaultButton（SC-005 行为不回归）
//   - 按钮文本沿用 Qt 标准映射（QMessageBox::standardButtonText）
// 工厂 showThemedMessageBox 为 QMessageBox 静态方法的直接替代
//（warning/question/information 语义与返回值保持一致）。
#pragma once

#include <QDialog>
#include <QMessageBox>

class QKeyEvent;
class QMouseEvent;

namespace perception {
namespace ui {

class ThemedMessageBox : public QDialog {
    Q_OBJECT
public:
    ThemedMessageBox(QWidget* parent, QMessageBox::Icon icon, const QString& title,
                     const QString& text,
                     QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                     QMessageBox::StandardButton defaultButton = QMessageBox::Ok);
    ~ThemedMessageBox() override = default;

    // exec() 返回后读取被点击的按钮；未点击任何按钮（关闭按钮 / Esc）时为 defaultButton
    QMessageBox::StandardButton clickedButton() const { return clicked_; }

protected:
    // 标题栏拖拽移动（去系统标题栏后自行处理，同 FramelessDialog）
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    // Esc → defaultButton（契约 §3：X/Esc 返回 defaultButton）
    void keyPressEvent(QKeyEvent* event) override;

private:
    QWidget* titleBar_ = nullptr;
    QPoint dragOffset_;
    QMessageBox::StandardButton clicked_ = QMessageBox::Ok;
};

// 模态显示并返回被点击按钮（替代 QMessageBox::warning/question/... 静态调用）
QMessageBox::StandardButton showThemedMessageBox(
    QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons = QMessageBox::Ok,
    QMessageBox::StandardButton defaultButton = QMessageBox::Ok);

}  // namespace ui
}  // namespace perception

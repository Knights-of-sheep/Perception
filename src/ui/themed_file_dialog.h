// ===== 主题化文件/目录对话框（006-constitution-refactor：自 MainWindow 提取）=====
// 背景：QFileDialog::getOpenFileName 等静态接口即使 setOption(DontUseNativeDialog)
//   也是系统标题栏，且 Qt 5.15 公开 API 无 setTitleBarWidget（仅 QDockWidget 有）。
//   而 ui-guidelines §4.3 又要求禁原生对话框以贴合 Qt 风格。
// 方案：Qt::FramelessWindowHint 的 QDialog 包裹 QFileDialog（Qt::Widget 化嵌入）：
//   自定义标题栏走 buildDialogTitleBar；QFileDialog 的 OK/Cancel 信号映射到 accept/reject。
// 该模块被 MainWindow 与 LogSettingsController（设置日志路径，FR-016）共用。
#pragma once

#include <QDialog>
#include <QPoint>
#include <QString>

class QFileDialog;
class QMouseEvent;
class QWidget;

namespace perception {
namespace ui {

enum class FileDialogMode { Open, Save, Directory };

// 无边框文件/目录对话框：自定义标题栏 + 内嵌 QFileDialog。
class ThemedFileDialog : public QDialog {
public:
    ThemedFileDialog(QWidget* parent, const QString& title,
                     const QString& dir, const QString& filter,
                     FileDialogMode mode);

    QString selectedFile() const;

protected:
    // 标题栏拖拽移动（去系统标题栏后自行处理）
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QWidget* titleBar_ = nullptr;
    QFileDialog* fileDialog_ = nullptr;
    QPoint dragOffset_;
};

// 文件/目录对话框统一入口（Open / Save / Directory）。
QString runThemedFileDialog(QWidget* parent, const QString& title,
                            const QString& dir, const QString& filter,
                            FileDialogMode mode);

}  // namespace ui
}  // namespace perception

// ===== 日志输出面板（M4：统一日志模块，FR-006 承载）=====
// 只读日志输出控件，ConsoleSink 的 UI 承载。
// 与底部 Python 控制台（REPL）分离：C++ 日志 / Python logging / Qt 消息重定向
// 统一在此显示，不再混入 py shell，避免干扰 Python 交互输出（FR-006 修订）。
#pragma once

#include <QColor>
#include <QPlainTextEdit>

namespace perception {
namespace ui {

class LogConsoleWidget final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit LogConsoleWidget(QWidget* parent = nullptr);

    // 追加一行日志（带级别颜色），可含换行；仅在 GUI 线程调用（由 ConsoleSink 队列投递）
    void appendOutput(const QString& text, const QColor& color);
};

}  // namespace ui
}  // namespace perception

#pragma once

#include <QColor>
#include <QObject>
#include <QString>

// Qt 的 qobjectdefs.h 将 emit 定义为空宏，会破坏 LogSink::emit 虚函数声明。
// 必须在 Qt 头之后、log_sink.h 之前 #undef；发射信号统一用 Q_EMIT（不受影响）。
#if defined(emit)
#undef emit
#endif

#include "core/log/log_sink.h"

namespace perception {
namespace ui {

class LogConsoleWidget;

// ConsoleLogSink：将统一日志实时投递到专属"日志输出"面板（LogConsoleWidget，FR-006）。
// 修订记录：早期版本投递到底部 PythonConsole（REPL），导致 C++ 日志混入 Python
//           交互输出；现分离为独立日志面板，py shell 只保留 Python 交互内容。
// 线程模型：emit() 可从任意线程调用（C++ 线程 / Python 线程），
//           内部经 Qt 队列（QueuedConnection）投递到 GUI 线程执行 appendOutput，
//           保证控制台文本操作只发生在 GUI 线程（FR-007）。
// 主题色：DEBUG 次要文字、INFO 主文字、WARN 警示色、ERROR/FATAL 危险色
//         （与现有错误样式一致；quickstart V1）。
class ConsoleLogSink final : public QObject, public perception::core::log::LogSink {
    Q_OBJECT

public:
    explicit ConsoleLogSink(LogConsoleWidget* console, QObject* parent = nullptr);
    ~ConsoleLogSink() override;

    void emit(const perception::core::log::LogRecord& record) override;
    const char* name() const noexcept override { return "ConsoleSink"; }

signals:
    void appendRequested(const QString& text, const QColor& color);

private:
    LogConsoleWidget* console_ = nullptr;
};

}  // namespace ui
}  // namespace perception

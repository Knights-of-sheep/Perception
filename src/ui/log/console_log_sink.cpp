#include "ui/log/console_log_sink.h"

#include "ui/log/log_console_widget.h"
#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"

#include <QMetaObject>
#include <Qt>

#include <chrono>
#include <cstdio>
#include <ctime>

namespace perception {
namespace ui {

namespace {

// 控制台行格式：HH:MM:SS.mmm LEVEL [source] message（quickstart V1 短格式）
QString formatConsoleLine(const perception::core::log::LogRecord& r)
{
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        r.timestamp.time_since_epoch()).count() % 1000;
    const std::time_t t = std::chrono::system_clock::to_time_t(r.timestamp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
                  tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms));
    return QString::fromUtf8(buf) + QLatin1Char(' ')
         + QLatin1String(perception::core::log::toString(r.level))
         + QLatin1String(" [") + QString::fromUtf8(r.source.c_str())
         + QLatin1String("] ") + QString::fromUtf8(r.message.c_str());
}

QColor colorForLevel(perception::core::log::LogLevel level)
{
    const auto& c = ThemeManager::current()->colors;  // current() 返回 ThemeDescriptor*
    switch (level) {
        case perception::core::log::LogLevel::Debug: return c.textWeak;
        case perception::core::log::LogLevel::Warn:  return c.warning;
        case perception::core::log::LogLevel::Error:
        case perception::core::log::LogLevel::Fatal: return c.danger;
        case perception::core::log::LogLevel::Info:
        default:                                     return c.text;
    }
}

}  // namespace

ConsoleLogSink::ConsoleLogSink(LogConsoleWidget* console, QObject* parent)
    : QObject(parent)
    , console_(console)
{
    // 队列连接：无论 emit 来自哪个线程，appendOutput 都只在 GUI 线程执行
    connect(this, &ConsoleLogSink::appendRequested, this,
            [this](const QString& text, const QColor& color) {
                if (console_)
                    console_->appendOutput(text, color);
            },
            Qt::QueuedConnection);
}

ConsoleLogSink::~ConsoleLogSink() = default;

void ConsoleLogSink::emit(const perception::core::log::LogRecord& record)
{
    const QString line = formatConsoleLine(record);
    const QColor color = colorForLevel(record.level);
    Q_EMIT appendRequested(line + QLatin1Char('\n'), color);
}

}  // namespace ui
}  // namespace perception

#include "ui/log/qt_message_bridge.h"

#include "core/log/logger.h"

#include <QString>
#include <QtGlobal>

#include <cstdio>

namespace perception {
namespace ui {

namespace {

// 递归防护：Qt 消息回调内嵌套调用统一日志 API 时，最多一层跳转后直通 stderr。
thread_local bool s_inHandler = false;

perception::core::log::LogLevel mapQtMsgType(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:    return perception::core::log::LogLevel::Debug;
        case QtInfoMsg:     return perception::core::log::LogLevel::Info;
        case QtWarningMsg:  return perception::core::log::LogLevel::Warn;
        case QtCriticalMsg: return perception::core::log::LogLevel::Error;
        case QtFatalMsg:    return perception::core::log::LogLevel::Fatal;
    }
    return perception::core::log::LogLevel::Info;
}

// 来源格式：qt:<basename>:<line>（契约 cpp-logger-api.md §来源）
std::string qtSource(const char* file, int line)
{
    std::string f = file ? file : "qt";
    const std::size_t slash = f.find_last_of("/\\");
    if (slash != std::string::npos)
        f = f.substr(slash + 1);
    return "qt:" + f + ":" + std::to_string(line);
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (s_inHandler) {
        // 递归防护：直通 stderr，避免无限递归
        std::fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
        std::fflush(stderr);
        return;
    }
    s_inHandler = true;

    const std::string source = qtSource(context.file, context.line);
    const std::string text = msg.toUtf8().constData();
    perception::core::log::Logger::instance().log(mapQtMsgType(type), source, text);

    s_inHandler = false;
}

}  // namespace

QtMessageHandler installQtMessageBridge()
{
    const QtMessageHandler previous = qInstallMessageHandler(&qtMessageHandler);
    return previous;
}

void restoreQtMessageBridge(QtMessageHandler previous)
{
    qInstallMessageHandler(previous);
}

}  // namespace ui
}  // namespace perception

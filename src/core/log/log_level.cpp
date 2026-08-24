#include "core/log/log_level.h"

#include "core/log/logger.h"

#include <cctype>
#include <string>

namespace perception::core::log {

const char* toString(LogLevel level) noexcept
{
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "INFO";  // 防御：未知值
}

LogLevel parseLevel(const std::string& s) noexcept
{
    std::string upper;
    upper.reserve(s.size());
    for (char c : s)
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

    if (upper == "DEBUG") return LogLevel::Debug;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::Warn;
    if (upper == "ERROR") return LogLevel::Error;
    if (upper == "FATAL" || upper == "CRITICAL") return LogLevel::Fatal;
    if (upper == "INFO") return LogLevel::Info;

    // 未知值回退 Info 并告警（FR：解析容错）
    Logger::instance().warnAt(__FILE__, __LINE__,
                              "parseLevel: unknown level '" + s + "', fallback to INFO");
    return LogLevel::Info;
}

} // namespace perception::core::log

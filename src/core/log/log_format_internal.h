#pragma once

// 内部共享：统一日志行格式（契约 log-file-format.md）。
// 被 FileSink / TerminalSink 复用，保证终端与文件行格式完全一致（FR-002「保持一致」）。
// 采用 inline 函数：perception_core 开启 Unity Build 时多 TU 合并，不产生重定义冲突。

#include "core/log/log_level.h"
#include "core/log/log_record.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

namespace perception::core::log::detail {

// YYYY-MM-DD HH:MM:SS.mmm
inline std::string formatTimestamp(const std::chrono::system_clock::time_point& tp)
{
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        tp.time_since_epoch()).count() % 1000;
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms));
    return buf;
}

// 级别固定 5 字符右对齐补空格（契约 log-file-format.md：`WARN `）
inline std::string paddedLevel(LogLevel level)
{
    std::string s = toString(level);
    while (s.size() < 5)
        s.push_back(' ');
    return s;
}

// 完整一行（不含结尾换行）：YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message
inline std::string formatLine(const LogRecord& record)
{
    std::string line;
    line.reserve(32 + 5 + record.source.size() + record.message.size() + 4);
    line.append(formatTimestamp(record.timestamp));
    line.push_back(' ');
    line.append(paddedLevel(record.level));
    line.append(" [");
    line.append(record.source);
    line.append("] ");
    line.append(record.message);
    return line;
}

} // namespace perception::core::log::detail

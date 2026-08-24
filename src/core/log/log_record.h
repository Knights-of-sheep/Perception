#pragma once

#include "core/log/log_level.h"

#include <chrono>
#include <string>

namespace perception::core::log {

// 单条日志记录的数据契约。只读；由 Logger 构造并广播。
struct LogRecord {
    std::chrono::system_clock::time_point timestamp; // 本地时间，毫秒精度（FR-003）
    LogLevel level;                                  // 级别
    std::string source;                              // 来源：C++ 为 "file:line"；Python 为 "name:lineno"；无来源为 "core"
    std::string message;                             // 消息正文，UTF-8；不要求含换行（Formatter 负责加 '\n'）
};

} // namespace perception::core::log

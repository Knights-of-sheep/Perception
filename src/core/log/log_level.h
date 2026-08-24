#pragma once

#include <string>

namespace perception::core::log {

// 日志级别值域。每级别独立开关（FR-002），非单一阈值。
enum class LogLevel : int {
    Debug = 0,
    Info,
    Warn,
    Error,
    Fatal,
};

// 稳定字符串（文件/控制台统一大写）："DEBUG"/"INFO"/"WARN"/"ERROR"/"FATAL"
const char* toString(LogLevel level) noexcept;

// 大小写不敏感解析；未知值回退 Info 并告警（经 Logger）
LogLevel parseLevel(const std::string& s) noexcept;

} // namespace perception::core::log

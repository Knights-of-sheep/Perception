#include "core/log/terminal_sink.h"

#include "core/log/log_format_internal.h"

#include <chrono>
#include <cstdio>
#include <ctime>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace perception::core::log {

namespace {

// 级别 ANSI 颜色（SGR；名称带 Terminal 前缀，避免 Unity Build 合并 TU 时与其它 sink 冲突）
constexpr const char* kTerminalAnsiDebug = "\x1b[90m"; // 亮黑/灰
constexpr const char* kTerminalAnsiInfo  = "\x1b[0m";  // 默认
constexpr const char* kTerminalAnsiWarn  = "\x1b[33m"; // 黄
constexpr const char* kTerminalAnsiError = "\x1b[31m"; // 红
constexpr const char* kTerminalAnsiFatal = "\x1b[91m"; // 亮红
constexpr const char* kTerminalAnsiReset = "\x1b[0m";

const char* terminalAnsiFor(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug: return kTerminalAnsiDebug;
        case LogLevel::Warn:  return kTerminalAnsiWarn;
        case LogLevel::Error: return kTerminalAnsiError;
        case LogLevel::Fatal: return kTerminalAnsiFatal;
        case LogLevel::Info:
        default:              return kTerminalAnsiInfo;
    }
}

// 目标流：INFO/DEBUG → stdout；WARN 及以上 → stderr（便于管道分流）
FILE* terminalStreamFor(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:
        case LogLevel::Info:
            return stdout;
        case LogLevel::Warn:
        case LogLevel::Error:
        case LogLevel::Fatal:
        default:
            return stderr;
    }
}

} // namespace

TerminalSink::TerminalSink()
    : ansi_(false)
{
#ifdef _WIN32
    // Windows 10 1809+ 控制台支持 ANSI VT；显式开启（ENABLE_VIRTUAL_TERMINAL_PROCESSING）
    const HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && ::GetConsoleMode(hOut, &mode)) {
        constexpr DWORD kEnableVt = 0x0004;
        ::SetConsoleMode(hOut, mode | kEnableVt);
        ansi_ = true;
    }
#else
    // POSIX：tty 默认支持 ANSI；非 tty（重定向）时转义序列无害，保持开启
    ansi_ = true;
#endif
}

void TerminalSink::emit(const LogRecord& record)
{
    // 格式：YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message\n（共享 detail::formatLine）
    const std::string line = detail::formatLine(record) + "\n";

    const char* ansi = ansi_ ? terminalAnsiFor(record.level) : "";
    writeLine(terminalStreamFor(record.level), line, ansi);
}

void TerminalSink::writeLine(FILE* stream, const std::string& line, const char* ansi)
{
    std::lock_guard<std::mutex> lock(mutex_);

#ifdef _WIN32
    const HANDLE h = ::GetStdHandle(stream == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    const bool isConsole = (h != INVALID_HANDLE_VALUE && ::GetConsoleMode(h, &mode));
    if (isConsole) {
        // 控制台：UTF-8 -> UTF-16 -> WriteConsoleW，与代码页无关，中文不乱码
        const std::string decorated = (ansi && *ansi) ? (ansi + line + kTerminalAnsiReset) : line;
        const int wlen = ::MultiByteToWideChar(CP_UTF8, 0, decorated.c_str(),
                                               static_cast<int>(decorated.size()), nullptr, 0);
        if (wlen > 0) {
            std::wstring wide(static_cast<std::size_t>(wlen), L'\0');
            ::MultiByteToWideChar(CP_UTF8, 0, decorated.c_str(),
                                  static_cast<int>(decorated.size()), wide.data(), wlen);
            DWORD written = 0;
            ::WriteConsoleW(h, wide.c_str(), static_cast<DWORD>(wide.size()), &written, nullptr);
        }
        return;
    }
#endif

    // 重定向/非控制台：退化为 UTF-8 字节直写（ANSI 转义照常输出，无害）
    if (ansi && *ansi)
        std::fwrite(ansi, 1, std::char_traits<char>::length(ansi), stream);
    std::fwrite(line.data(), 1, line.size(), stream);
    if (ansi && *ansi)
        std::fwrite(kTerminalAnsiReset, 1, std::char_traits<char>::length(kTerminalAnsiReset), stream);
    std::fflush(stream);
}

} // namespace perception::core::log

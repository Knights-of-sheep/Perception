#pragma once

#include "core/log/log_sink.h"

#include <mutex>

namespace perception::core::log {

// TerminalSink：将统一日志实时输出到进程终端（stdout/stderr，FR-006「控制台=终端」）。
//
// 行为约定：
//   - 行格式与 FileSink 完全一致：YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message\n
//     （FR-002 全局矩阵下终端与文件级别一致，输出格式亦保持单一样式契约 log-file-format.md）
//   - 级别着色：DEBUG 灰 / INFO 默认 / WARN 黄 / ERROR 红 / FATAL 亮红
//     （Windows 10 1809+ 控制台启用 ANSI VT；POSIX tty 默认支持）
//   - 目标流：INFO/DEBUG → stdout；WARN/ERROR/FATAL → stderr（便于管道分流）
//   - Windows 上控制台输出走 WriteConsoleW（UTF-16，与代码页无关，中文不乱码）；
//     被重定向到文件/管道时退化为 UTF-8 字节写入，与 FileSink 一致。
//   - 无终端（GUI 直接双击且无控制台）时静默丢弃，不抛异常。
//
// 线程安全：emit 可任意线程调用；内部 mutex 保证单条记录原子写出（FR-007）。
class TerminalSink final : public LogSink {
public:
    TerminalSink();
    ~TerminalSink() override = default;

    void emit(const LogRecord& record) override;
    const char* name() const noexcept override { return "TerminalSink"; }

private:
    // 写入一行到指定流；ansi 为 true 时包裹颜色码。
    void writeLine(FILE* stream, const std::string& line, const char* ansi);

    std::mutex mutex_;
    bool ansi_ = false; // 当前进程是否启用 ANSI 颜色
};

} // namespace perception::core::log

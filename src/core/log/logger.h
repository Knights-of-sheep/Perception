#pragma once

#include "core/log/log_level.h"
#include "core/log/log_level_matrix.h"
#include "core/log/log_record.h"
#include "core/log/log_sink.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace perception::core::log {

// 统一日志 API 单例：按各 sink 的 LogLevelMatrix 过滤后广播记录。
class Logger {
public:
    // 单例
    static Logger& instance();

    // ---- 配置（FR-002/004/005/011；启动时调用一次即可）----
    struct Config {
        std::string filePath;          // 空 = 不写文件；默认 ""
        LogLevelMatrix levelMatrix;    // 默认矩阵：DEBUG 关、其余开；作为 FileSink 初始矩阵与 UI 初始态（FR-002）
        bool vtkLoggingEnabled = true; // VTK 日志纳入开关（FR-011）；当前仅占位，VTK 落地后启用
        std::uint64_t maxFileSize = 5 * 1024 * 1024;  // 5MB
        int maxBackups = 3;            // 归档份数
    };
    void configure(const Config& cfg); // 线程安全；可重复调用（更新配置）

    // ---- 级别写入（FR-001）----
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);
    void log(LogLevel level, const std::string& msg);

    // ---- 显式来源（Python 桥接等：source 形如 "name:lineno"；FR-008）----
    void log(LogLevel level, const std::string& source, const std::string& msg);

    // ---- 来源宏：自动携带 file:line（FR-003）----
    void log(LogLevel level, const char* file, int line, const std::string& msg);
    void debugAt(const char* file, int line, const std::string& msg);
    void infoAt(const char* file, int line, const std::string& msg);
    void warnAt(const char* file, int line, const std::string& msg);
    void errorAt(const char* file, int line, const std::string& msg);
    void fatalAt(const char* file, int line, const std::string& msg);

    // ---- 级别控制（FR-002）----
    // 无全局 setLevel：按各 sink 的 LogLevelMatrix 过滤；运行时持 sink 指针调 setLevelEnabled。
    // FR-002 全局矩阵：UI 遍历 sinks() 同步全部 sink。GUI 输出目标=终端+文件（无内置日志面板）。
    const LogLevelMatrix& defaultMatrix() const noexcept;

    // ---- sink 管理（扩展点）----
    void addSink(LogSinkPtr sink);
    void removeSink(LogSinkPtr sink);
    std::size_t sinkCount() const noexcept;
    // 按名称查找已注册 sink（用于 UI 双向同步；未找到返回 nullptr）
    LogSinkPtr findSink(const char* name) const;
    // 已注册 sink 快照（用于 UI 全局级别切换时遍历全部 sink，保持矩阵一致；FR-002）
    std::vector<LogSinkPtr> sinks() const;

    // ---- 运行期日志路径管理（设置菜单：日志路径可配置 + 清除历史日志；FR-016/017）----
    // 切换文件路径：旧日志（app.log 及归档）迁移到新路径所在目录后重建 FileSink，
    // 保留原级别矩阵与 sink 指针。返回新路径是否可写（false = 未启用文件输出）。
    bool setFilePath(const std::string& newPath);
    // 清除当前日志文件与全部归档（关闭句柄后删除并重建空文件），返回是否可写。
    bool clearLogFiles();
    // 当前文件路径（空 = 未启用文件输出）
    const std::string& filePath() const noexcept;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace perception::core::log

// 便捷宏：自动携带 file:line。禁用宏时可定义 PERCEPTION_LOG_DISABLE_MACROS。
#ifndef PERCEPTION_LOG_DISABLE_MACROS
#define PERCEPTION_LOG_D(...) \
    ::perception::core::log::Logger::instance().debugAt(__FILE__, __LINE__, __VA_ARGS__)
#define PERCEPTION_LOG_I(...) \
    ::perception::core::log::Logger::instance().infoAt(__FILE__, __LINE__, __VA_ARGS__)
#define PERCEPTION_LOG_W(...) \
    ::perception::core::log::Logger::instance().warnAt(__FILE__, __LINE__, __VA_ARGS__)
#define PERCEPTION_LOG_E(...) \
    ::perception::core::log::Logger::instance().errorAt(__FILE__, __LINE__, __VA_ARGS__)
#define PERCEPTION_LOG_F(...) \
    ::perception::core::log::Logger::instance().fatalAt(__FILE__, __LINE__, __VA_ARGS__)
#endif // PERCEPTION_LOG_DISABLE_MACROS

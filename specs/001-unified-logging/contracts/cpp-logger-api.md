# Contract: C++ 统一日志 API

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](../spec.md) | **Data Model**: [data-model.md](../data-model.md)

本项目为桌面应用，对外的主要"接口"是代码级 API。本契约定义 `src/core/log/` 的公共头文件接口，供任意 C++ 模块（core / ui / app）调用。

## 头文件与命名空间

```cpp
// 用户只需包含：
#include "core/log/logger.h"
#include "core/log/log_level.h"

// 扩展 sink 时包含：
#include "core/log/log_sink.h"
#include "core/log/log_record.h"
```

所有类型位于 `namespace perception::core::log`。

## LogLevel（log_level.h）

```cpp
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
```

## 日志调用（logger.h）

```cpp
class Logger {
public:
    // 单例
    static Logger& instance();

    // ---- 配置（FR-002/004/005；启动时调用一次即可）----
    struct Config {
        std::string filePath;          // 空 = 不写文件；默认 ""
        LogLevel level = LogLevel::Info;
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

    // ---- 来源宏：自动携带 file:line（FR-003）----
    void log(LogLevel level, const char* file, int line, const std::string& msg);
    void debugAt(const char* file, int line, const std::string& msg);
    void infoAt(const char* file, int line, const std::string& msg);
    void warnAt(const char* file, int line, const std::string& msg);
    void errorAt(const char* file, int line, const std::string& msg);
    void fatalAt(const char* file, int line, const std::string& msg);

    // ---- 级别查询 ----
    LogLevel level() const noexcept;
    void setLevel(LogLevel level) noexcept;  // 热更新阈值

    // ---- sink 管理（扩展点）----
    void addSink(std::shared_ptr<LogSink> sink);
    void removeSink(std::shared_ptr<LogSink> sink);
    std::size_t sinkCount() const noexcept;

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};
```

**便捷宏**（`logger.h` 内，禁用宏时可 `#define PERCEPTION_LOG_DISABLE_MACROS`）：

```cpp
#define PERCEPTION_LOG_D(...) \
    ::perception::core::log::Logger::instance().debugAt(__FILE__, __LINE__, ...)
#define PERCEPTION_LOG_I(...) /* info */
#define PERCEPTION_LOG_W(...) /* warn */
#define PERCEPTION_LOG_E(...) /* error */
#define PERCEPTION_LOG_F(...) /* fatal */
```

## 扩展 sink（log_sink.h）

```cpp
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void emit(const LogRecord& record) = 0;   // 不得抛异常
    virtual const char* name() const noexcept = 0;
};
```

## FileSink（file_sink.h，core 内置）

```cpp
// 由 Logger::configure(filePath 非空) 自动创建并注册；通常无需手动构造。
// 手动场景（测试）：可独立构造并喂记录。
class FileSink : public LogSink {
public:
    FileSink(std::string path,
             std::uint64_t maxFileSize = 5 * 1024 * 1024,
             int maxBackups = 3);
    ~FileSink() override;
    void emit(const LogRecord& record) override;   // FR-005 轮转内嵌
    const char* name() const noexcept override;
};
```

## 语义约定

1. **线程安全**：`Logger` 全部公开方法线程安全（内部锁）；`emit` 广播在锁内遍历副本。
2. **无异常逃逸**：`log(...)` 族不抛异常（内部捕获 sink 异常，FR-009）。
3. **来源**：`debugAt(file, line, ...)` 生成 `source = "<basename>:<line>"`；无来源的重载记作 `source = "<core>"`。
4. **UTF-8**：`std::string` 消息按 UTF-8 解释（宪法 Design Constraints）。
5. **广播顺序**：sink 按注册顺序接收同一记录；文件与控制台内容一致（同一格式化规则）。

# Contract: C++ 统一日志 API

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](../spec.md) | **Data Model**: [data-model.md](../data-model.md)

本项目为桌面应用，对外的主要"接口"是代码级 API。本契约定义 `src/core/log/` 的公共头文件接口，供任意 C++ 模块（core / ui / app）调用。

## 头文件与命名空间

```cpp
// 用户只需包含：
#include "core/log/logger.h"
#include "core/log/log_level.h"

// 扩展 sink / 配置级别矩阵时包含：
#include "core/log/log_level_matrix.h"
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

## LogLevelMatrix（log_level_matrix.h，FR-002）

每级别独立开关集合；控制台与文件各持一个独立实例。

```cpp
class LogLevelMatrix {
public:
    LogLevelMatrix();   // 默认：DEBUG 关，INFO/WARN/ERROR/FATAL 开

    void setEnabled(LogLevel level, bool enabled);
    bool isEnabled(LogLevel level) const noexcept;

    // 一次打开/关闭全部级别（测试与 UI 重置用）
    void setAll(bool enabled);

private:
    std::bitset<5> bits_;   // 索引 = static_cast<int>(LogLevel)
};
```

## 日志调用（logger.h）

```cpp
class Logger {
public:
    // 单例
    static Logger& instance();

    // ---- 配置（FR-002/004/005/011；启动时调用一次即可）----
    struct Config {
        std::string filePath;          // 空 = 不写文件；默认 ""
        LogLevelMatrix levelMatrix;    // 默认矩阵：DEBUG 关、其余开；作为 FileSink 初始矩阵与 UI 初始态（FR-002）
        bool vtkLoggingEnabled = true; // VTK 日志纳入开关（FR-011）
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

    // ---- 级别控制（FR-002）----
    // 无全局 setLevel：过滤由广播路径按各 sink 的 LogLevelMatrix 执行。
    // 运行时切换级别：持有 sink 指针（ConsoleSink/FileSink）后调 sink->setLevelEnabled(...)。
    const LogLevelMatrix& defaultMatrix() const noexcept;

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

    // ---- 级别矩阵（FR-002）：每 sink 独立；控制台与文件可分别配置 ----
    void setLevelEnabled(LogLevel level, bool enabled);
    bool isLevelEnabled(LogLevel level) const noexcept;
    const LogLevelMatrix& levelMatrix() const noexcept;

    virtual void emit(const LogRecord& record) = 0;   // 不得抛异常
    virtual const char* name() const noexcept = 0;

protected:
    LogLevelMatrix matrix_;   // 默认 DEBUG 关、其余开
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
2. **级别过滤**（FR-002）：广播时对每个已注册 sink，若 `!sink->isLevelEnabled(record.level)` 则跳过该 sink（`emit` 不被调用）；配置归属 sink 矩阵、执行位于 Logger 广播路径。
3. **无异常逃逸**：`log(...)` 族不抛异常（内部捕获 sink 异常，FR-009）。
4. **来源**：`debugAt(file, line, ...)` 生成 `source = "<basename>:<line>"`；无来源的重载记作 `source = "<core>"`；Qt 重定向为 `qt:<basename>:<line>`（FR-010）；VTK 为 `vtk:...`（FR-011）。
5. **UTF-8**：`std::string` 消息按 UTF-8 解释（宪法 Design Constraints）。
6. **广播顺序**：sink 按注册顺序接收同一记录；文件与控制台内容一致（同一格式化规则）。
